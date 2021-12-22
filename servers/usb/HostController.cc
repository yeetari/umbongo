#include "HostController.hh"

#include "Common.hh"
#include "Context.hh"
#include "Descriptor.hh"
#include "Device.hh"
#include "Error.hh"
#include "KeyboardDevice.hh"
#include "Port.hh"
#include "TrbRing.hh"

#include <core/Error.hh>
#include <core/File.hh>
#include <core/Process.hh>
#include <kernel/SyscallTypes.hh>
#include <mmio/Mmio.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/ScopeGuard.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

struct [[gnu::packed]] EventRingSegment {
    uint64 base;
    uint32 trb_count;
    uint32 : 32;
};

} // namespace

HostController::HostController(ustd::StringView name, core::File &&file) : m_name(name), m_file(ustd::move(file)) {}
HostController::HostController(HostController &&) noexcept = default;
HostController::~HostController() = default;

ustd::Result<void, ustd::ErrorUnion<core::SysError, HostError>> HostController::enable() {
    TRY(m_file.ioctl(kernel::IoctlRequest::PciEnableDevice));
    auto *cap_regs = TRY(m_file.mmap<CapRegs>());

    const auto op_base = reinterpret_cast<uintptr>(cap_regs) + mmio::read(cap_regs->cap_length);
    const auto rt_base = reinterpret_cast<uintptr>(cap_regs) + mmio::read(cap_regs->runtime_offset);
    const auto db_base = reinterpret_cast<uintptr>(cap_regs) + mmio::read(cap_regs->doorbell_offset);
    ENSURE(op_base % sizeof(uint32) == 0);
    ENSURE(rt_base % 32 == 0);
    ENSURE(db_base % sizeof(uint32) == 0);
    m_op_regs = reinterpret_cast<OpRegs *>(op_base);
    m_run_regs = reinterpret_cast<RunRegs *>(rt_base);
    m_doorbell_array = reinterpret_cast<DoorbellArray *>(db_base);

    // Ensure the run/stop bit is clear and wait until halted bit is set. Spec says HC should halt within 16 ms, but
    // linux uses 32 ms "as some hosts take longer".
    mmio::write(m_op_regs->command, mmio::read(m_op_regs->command) & ~1u);
    if (!mmio::wait_timeout(m_op_regs->status, 1u, 1u, 32)) {
        return HostError::HaltTimedOut;
    }

    // Reset the controller and unconditionally wait 1 ms to allow older intel HCs to reset properly.
    mmio::write(m_op_regs->command, mmio::read(m_op_regs->command) | (1u << 1u));
    core::sleep(1000000ul);
    if (!mmio::wait_timeout(m_op_regs->command, (1u << 1u), 0u, 10000)) {
        return HostError::ResetTimedOut;
    }

    // Wait for controller readiness after reset.
    if (!mmio::wait_timeout(m_op_regs->status, (1u << 11u), 0u, 10000)) {
        return HostError::ResetTimedOut;
    }

    // TODO: Support other page sizes?
    if (mmio::read(m_op_regs->page_size) != 1u) {
        return HostError::UnsupportedPageSize;
    }

    // Retrieve context size and ensure that 64-bit addressing (bit 0) is available from capability parameters.
    m_context_size = (mmio::read(cap_regs->hcc_params1) & (1u << 2u)) == 0u ? 32u : 64u;
    if ((mmio::read(cap_regs->hcc_params1) & 1u) != 1u) {
        return HostError::No64BitAddressing;
    }

    // Program max slot count.
    const auto config = mmio::read(m_op_regs->config) & ~0xffu;
    const auto slot_count = mmio::read(cap_regs->hcs_params1) & 0xffu;
    mmio::write(m_op_regs->config, config | slot_count);

    // Allocate the device context base address array. `+ 1` for the scratchpad.
    m_context_table = TRY(mmio::alloc_dma_array<uintptr>(slot_count + 1));
    m_devices.ensure_size(slot_count);
    mmio::write(m_op_regs->dcbaap, EXPECT(mmio::virt_to_phys(m_context_table)));

    // Allocate the scratchpad buffers.
    const auto hcs_params2 = mmio::read(cap_regs->hcs_params2);
    uint32 scratch_buffer_count = ((hcs_params2 >> 27u) & 0x1fu) | ((hcs_params2 >> 16u) & 0x3e0u);
    auto *scratchpad_buffer_array = TRY(mmio::alloc_dma_array<uintptr>(scratch_buffer_count));
    m_context_table[0] = EXPECT(mmio::virt_to_phys(scratchpad_buffer_array));
    for (uint32 i = 0; i < scratch_buffer_count; i++) {
        auto *scratchpad_buffer = TRY(mmio::alloc_dma_array<uint8>(4_KiB));
        scratchpad_buffer_array[i] = EXPECT(mmio::virt_to_phys(scratchpad_buffer));
    }

    // Allocate the command ring.
    auto command_ring = TRY(TrbRing::create(true));
    mmio::write(m_op_regs->crcr, EXPECT(command_ring->physical_base()) | 1u);

    // Allocate the event ring and set up the primary interrupter.
    auto event_ring = TRY(TrbRing::create(false));
    auto *segment_table = TRY(mmio::alloc_dma_array<EventRingSegment>(1));
    segment_table[0].base = EXPECT(event_ring->physical_base());
    segment_table[0].trb_count = 256; // TODO: Don't hardcode here.
    mmio::write(m_run_regs->imod, (mmio::read(m_run_regs->imod) & ~0xffffu) | 160u);
    mmio::write(m_run_regs->erst_size, (mmio::read(m_run_regs->erst_size) & ~0xffffu) | 1u);
    mmio::write(m_run_regs->erst_dptr, EXPECT(event_ring->physical_head()));
    mmio::write(m_run_regs->erst_base, EXPECT(mmio::virt_to_phys(segment_table)));

    const auto port_count = (mmio::read(cap_regs->hcs_params1) >> 24u) & 0xffu;
    m_ports.ensure_size(port_count, nullptr, static_cast<uint8>(0u));

    // Initialise port array by reading extended capabilities.
    // TODO: Tidy this up.
    uint16 xecp = (mmio::read(cap_regs->hcc_params1) >> 16u) & 0xffffu;
    uint32 ext_cap = 0;
    do {
        ext_cap = mmio::read(reinterpret_cast<const uint32 *>(cap_regs)[xecp]);
        if ((ext_cap & 0xffu) != 2u) {
            continue;
        }
        auto protcol_info = mmio::read(reinterpret_cast<const uint32 *>(cap_regs)[xecp + 2]);
        uint8 protocol_port_start = (protcol_info >> 0u) & 0xffu;
        uint8 protocol_port_count = (protcol_info >> 8u) & 0xffu;
        for (uint8 i = protocol_port_start; i < (protocol_port_start + protocol_port_count); i++) {
            auto *portsc = reinterpret_cast<uint32 *>(op_base + 0x400 + (i - 1) * 0x10);
            m_ports[i - 1] = Port(portsc, i);
        }
        xecp += (ext_cap >> 8u) & 0xffu;
    } while (((ext_cap >> 8u) & 0xffu) != 0u);

    // Enable interrupts and start the schedule by
    //  1. Enabling PCI MSI/MSI-X via the ioctl
    //  2. Enabling system bus interrupt generation by setting the Interrupter Enable (INTE) bit
    //  3. Enabling the interrupter by setting the Interrupt Enable (IE) bit
    //  4. Setting the run/stop bit and waiting for the halted bit to clear
    TRY(m_file.ioctl(kernel::IoctlRequest::PciEnableInterrupts));
    mmio::write(m_op_regs->command, mmio::read(m_op_regs->command) | (1u << 2u));
    mmio::write(m_run_regs->iman, (mmio::read(m_run_regs->iman) & ~0b11u) | (1u << 1u));
    mmio::write(m_op_regs->command, mmio::read(m_op_regs->command) | 1u);
    if (!mmio::wait_timeout(m_op_regs->status, 1u, 0u, 32)) {
        return HostError::StartTimedOut;
    }

    // Now we know that initialisation has succeeded, we can take ownership of the rings.
    m_command_ring = ustd::move(command_ring);
    m_event_ring = ustd::move(event_ring);
    m_file.set_on_write_ready([this] {
        handle_interrupt();
    });
    for (auto &port : m_ports) {
        if (port.connected() && !port.reset()) {
            ustd::dbgln(" usb: Failed to reset port {}", 1);
        }
    }
    return {};
}

void HostController::handle_interrupt() {
    static_cast<void>(m_file.read({}));
    auto status = mmio::read(m_op_regs->status);
    if ((status & (1u << 3u)) == 0u) {
        return;
    }
    mmio::write(m_op_regs->status, status | (1u << 3u));

    bool had_any_event = false;
    for (auto &event : m_event_ring->dequeue()) {
        had_any_event = true;
        const uint8 completion_code = (event.status >> 24u) & 0xffu;
        if (completion_code != 1u) {
            ustd::dbgln(" usb: Received event {} with completion code {}", static_cast<uint8>(event.type),
                        completion_code);
            continue;
        }
        switch (event.type) {
        case TrbType::TransferEvent: {
            // TODO: Cache physical base.
            auto &device = *m_devices[event.slot_id - 1u];
            auto &transfer_ring = device.endpoint_ring(event.endpoint_id - 1u);
            usize trb_index = (event.data - EXPECT(transfer_ring.physical_base())) / sizeof(RawTrb);
            auto &transfer = transfer_ring[trb_index];
            if (transfer.type == TrbType::Normal) {
                transfer.cycle = !transfer.cycle;
                if (trb_index < 255u) {
                    if (auto &link = transfer_ring[trb_index + 1]; link.type == TrbType::Link) {
                        link.cycle = !link.cycle;
                    }
                }
                device.poll();
            }
            if (transfer.type == TrbType::StatusStage) {
                transfer.status |= 1u << 31u;
            }
            break;
        }
        case TrbType::CommandCompletionEvent: {
            // TODO: Cache physical base.
            usize command_index = (event.data - EXPECT(m_command_ring->physical_base())) / sizeof(RawTrb);
            auto &command = (*m_command_ring)[command_index];
            command.slot_id = event.slot_id;
            command.status = event.status;
            command.status |= 1u << 31u;
            break;
        }
        case TrbType::PortStatusChangeEvent: {
            const uint8 port_id = (event.data >> 24u) & 0xffu;
            // TODO: Better logging here.
            if (auto result = handle_port_status_change(event); result.is_error()) {
                auto error = result.error();
                if (auto device_error = error.as<DeviceError>()) {
                    ustd::dbgln(" usb: Failed to handle port status change for port {}: {}", port_id,
                                device_error_string(*device_error));
                } else if (auto host_error = error.as<HostError>()) {
                    ustd::dbgln(" usb: Failed to handle port status change for port {}: {}", port_id,
                                host_error_string(*host_error));
                }
            }
            break;
        }
        default:
            ustd::dbgln(" usb: Received unrecognised event {}", static_cast<uint8>(event.type));
            break;
        }
    }

    uint64 dptr = mmio::read(m_run_regs->erst_dptr);
    if (auto new_dptr = EXPECT(m_event_ring->physical_head()); (dptr & ~0xfu) != (new_dptr & ~0xfu)) {
        if (!had_any_event) {
            ustd::dbgln(" usb: Update dptr without dequeuing an event");
        }
        dptr &= 0xfu;
        dptr |= new_dptr & ~0xfu;
    } else if (had_any_event) {
        ustd::dbgln(" usb: Dequeued event but dptr didn't change");
    }
    mmio::write(m_run_regs->erst_dptr, dptr | (1u << 3u));
}

ustd::Result<void, ustd::ErrorUnion<DeviceError, HostError, core::SysError>>
HostController::handle_port_status_change(const RawTrb &event) {
    const uint8 port_id = (event.data >> 24u) & 0xffu;
    if (port_id == 0 || port_id > m_ports.size()) {
        return HostError::InvalidPortId;
    }
    auto &port = m_ports[port_id - 1];
    if (!port.connected()) {
        ENSURE_NOT_REACHED("TODO: Handle disconnect");
    }

    auto enable_slot = TRY(send_command({
        .type = TrbType::EnableSlotCmd,
    }));
    ustd::dbgln(" usb: Enabled slot {} for port {}", enable_slot.slot_id, port_id);

    auto &device_ptr = m_devices[enable_slot.slot_id - 1u];
    auto &device = device_ptr.emplace(*this, enable_slot.slot_id);
    auto *device_context = TRY(device.setup(port));
    m_context_table[enable_slot.slot_id] = EXPECT(mmio::virt_to_phys(device_context));
    ustd::ScopeGuard destroy_guard([&] {
        device_ptr.clear();
    });

    // TODO: Free input_context.
    void *input_context_ptr = nullptr;
    switch (m_context_size) {
    case 32: {
        auto *input_context = TRY(mmio::alloc_dma<InputContext>());
        input_context->control.add = 0b11u;
        __builtin_memcpy(&input_context->device, device_context, sizeof(DeviceContext));
        input_context_ptr = input_context;
        break;
    }
    case 64: {
        auto *input_context = TRY(mmio::alloc_dma<LargeInputContext>());
        input_context->control.add = 0b11u;
        __builtin_memcpy(&input_context->device, device_context, sizeof(LargeDeviceContext));
        input_context_ptr = input_context;
        break;
    }
    }

    TRY(send_command({
        .data = EXPECT(mmio::virt_to_phys(input_context_ptr)),
        .block = true,
        .type = TrbType::AddressDeviceCmd,
        .slot_id = enable_slot.slot_id,
    }));
    if (device.slot_state() != SlotState::Default) {
        return DeviceError::UnexpectedState;
    }

    ustd::Array<uint8, 8> partial_descriptor{};
    TRY(device.read_descriptor(partial_descriptor.span()));

    TRY(send_command({
        .data = EXPECT(mmio::virt_to_phys(input_context_ptr)),
        .type = TrbType::AddressDeviceCmd,
        .slot_id = enable_slot.slot_id,
    }));
    if (device.slot_state() != SlotState::Addressed) {
        return DeviceError::UnexpectedState;
    }

    const auto descriptor_length = partial_descriptor[0];
    if (descriptor_length < sizeof(DeviceDescriptor)) {
        return DeviceError::InvalidDescriptor;
    }

    ustd::Vector<uint8> descriptor_bytes(descriptor_length);
    TRY(device.read_descriptor(descriptor_bytes.span()));
    auto *descriptor = reinterpret_cast<DeviceDescriptor *>(descriptor_bytes.data());
    if (descriptor->dclass != 0u) {
        ustd::dbgln(" usb: Found device {:h2}:{:h2}:{:h2}", descriptor->dclass, descriptor->dsubclass,
                    descriptor->dprotocol);
        return DeviceError::NoDriverAvailable;
    }

    TRY(device.walk_configuration([](void *descriptor, DescriptorType type) -> ustd::Result<void, DeviceError> {
        if (type == DescriptorType::Interface) {
            auto *interface_descriptor = static_cast<InterfaceDescriptor *>(descriptor);
            ustd::dbgln(" usb: Found interface {:h2}:{:h2}:{:h2}", interface_descriptor->iclass,
                        interface_descriptor->isubclass, interface_descriptor->iprotocol);
            if (interface_descriptor->iclass != 3u || interface_descriptor->isubclass != 1u ||
                interface_descriptor->iprotocol != 1u) {
                return DeviceError::NoDriverAvailable;
            }
        }
        return {};
    }));

    // We have an available driver, so we can deactivate the device cleanup guard and effectively "promote" the device
    // to its derived driver class.
    destroy_guard.disarm();
    auto moved_device = ustd::move(device);
    auto &keyboard_device = device_ptr.emplace<KeyboardDevice>(ustd::move(moved_device));
    TRY(keyboard_device.enable());
    return {};
}

void HostController::ring_doorbell(uint32 slot, uint32 endpoint) {
    mmio::write(m_doorbell_array->doorbell[slot], endpoint);
    mmio::read(m_doorbell_array->doorbell[slot]);
}

ustd::Result<RawTrb, HostError> HostController::send_command(const RawTrb &command) {
    // TODO: Correct timeout.
    auto &enqueued = m_command_ring->enqueue(command);
    ring_doorbell(0, 0);
    for (usize timeout = 1000; (mmio::read(enqueued.status) & (1u << 31u)) != (1u << 31u); timeout--) {
        if (timeout == 0) {
            return HostError::CommandTimedOut;
        }
        // TODO: Better solution for this.
        handle_interrupt();
        core::sleep(1000000ul);
    }
    asm volatile("lfence" ::: "memory");
    return enqueued;
}
