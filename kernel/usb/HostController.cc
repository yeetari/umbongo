#include <kernel/usb/HostController.hh>

#include <kernel/cpu/RegisterState.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/usb/Descriptors.hh>
#include <kernel/usb/Device.hh>
#include <kernel/usb/Interrupter.hh>
#include <kernel/usb/Port.hh>
#include <kernel/usb/SlotState.hh>
#include <kernel/usb/TrbRing.hh>
#include <kernel/usb/UsbManager.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace usb {
namespace {

constexpr uint16 k_config_reg_sbrn = 0x60;

constexpr uint16 k_cap_reg_length = 0x00;
constexpr uint16 k_cap_reg_hcsparams1 = 0x04;
constexpr uint16 k_cap_reg_hcsparams2 = 0x08;
constexpr uint16 k_cap_reg_hccparams1 = 0x10;
constexpr uint16 k_cap_reg_db_offset = 0x14;
constexpr uint16 k_cap_reg_run_offset = 0x18;

constexpr uint16 k_op_reg_command = 0x00;
constexpr uint16 k_op_reg_status = 0x04;
constexpr uint16 k_op_reg_dnctrl = 0x14;
constexpr uint16 k_op_reg_crcr = 0x18;
constexpr uint16 k_op_reg_dcbapp = 0x30;
constexpr uint16 k_op_reg_config = 0x38;

void watch_loop(HostController *controller) {
    while (true) {
        for (auto &port : controller->ports()) {
            if (!port.status_changed()) {
                continue;
            }
            port.clear_status_changed();
            if (port.connected()) {
                if (!port.enabled()) {
                    if (!port.reset()) {
                        logln(" usb: Failed to reset port {}!", port.number());
                    }
                    continue;
                }
                logln(" usb: Device attached to port {}", port.number());
                controller->on_attach(port);
            } else {
                logln(" usb: Device removed from port {}", port.number());
                controller->on_detach(port);
            }
        }
        Scheduler::yield();
    }
}

} // namespace

template <typename T>
T HostController::read_cap(uint16 offset) const {
    ASSERT(offset % sizeof(uint32) == 0);
    return *reinterpret_cast<volatile T *>(m_base + offset);
}

template <typename T>
T HostController::read_op(uint16 offset) const {
    ASSERT(offset % sizeof(uint32) == 0);
    return *reinterpret_cast<volatile T *>(m_base + m_cap_length + offset);
}

template <typename T>
void HostController::write_cap(uint16 offset, T value) {
    ASSERT(offset % sizeof(uint32) == 0);
    *reinterpret_cast<volatile T *>(m_base + offset) = value;
}

template <typename T>
void HostController::write_op(uint16 offset, T value) {
    ASSERT(offset % sizeof(uint32) == 0);
    *reinterpret_cast<volatile T *>(m_base + m_cap_length + offset) = value;
}

void HostController::ring_doorbell(uint8 slot, uint8 endpoint) const {
    *reinterpret_cast<volatile uint32 *>(m_base + m_db_offset + slot * sizeof(uint32)) = endpoint;
}

void HostController::enable() {
    m_base = read_bar(0);
    m_cap_length = read_cap<uint8>(k_cap_reg_length);
    m_db_offset = read_cap<uint32>(k_cap_reg_db_offset) & ~0b11u;
    m_run_offset = read_cap<uint32>(k_cap_reg_run_offset) & ~0x1fu;

    const auto usb_spec = read_config<uint8>(k_config_reg_sbrn);
    logln(" usb: Host Controller is compliant with USB specification {}.{}", usb_spec >> 4u, usb_spec & 0xfu);

    // Ensure run/stop bit is clear and wait until halted bit set. We can only modify the config/control registers when
    // the controller is halted.
    write_op<uint32>(k_op_reg_command, read_op<uint32>(k_op_reg_command) & ~1u);
    while ((read_op<uint32>(k_op_reg_status) & 1u) == 0) {
        Scheduler::wait(1);
    }

    // Reset the controller by setting the reset bit and then waiting for the reset and controller not ready bits to be
    // clear.
    write_op<uint32>(k_op_reg_command, 1u << 1u);
    while ((read_op<uint32>(k_op_reg_command) & (1u << 1u)) != 0 ||
           (read_op<uint32>(k_op_reg_status) & (1u << 11u)) != 0) {
        Scheduler::wait(1);
    }

    // Retrieve capability and structural parameters to get the context size and ensure that 64-bit addressing (bit 0)
    // is available.
    const auto hcc_params1 = read_cap<uint32>(k_cap_reg_hccparams1);
    const auto hcs_params1 = read_cap<uint32>(k_cap_reg_hcsparams1);
    const auto hcs_params2 = read_cap<uint32>(k_cap_reg_hcsparams2);
    m_context_size = (hcc_params1 & (1u << 2u)) == 0 ? 32 : 64;
    ENSURE((hcc_params1 & (1u << 0u)) != 0, "64-bit addressing not available!");

    const uint8 slot_count = (hcs_params1 >> 0u) & 0xffu;
    const uint8 port_count = (hcs_params1 >> 24u) & 0xffu;
    m_ports.grow(port_count);

    // Iterate extended capabilities to build port info and relinquish BIOS control if present.
    uint16 xecp = (hcc_params1 >> 16u) & 0xffffu;
    ENSURE(xecp != 0);
    uint32 ext_cap = 0;
    do {
        ext_cap = read_cap<uint32>(xecp * sizeof(uint32));
        switch (ext_cap & 0xffu) {
        case 1:
            // Legacy Support - disable BIOS control. First set the System Software Owned Semaphore bit and then wait
            // for the BIOS to clear the BIOS Owned Semaphore bit.
            write_cap<uint32>(xecp * sizeof(uint32), ext_cap | (1u << 24u));
            while ((read_cap<uint32>(xecp * sizeof(uint32)) & (1u << 16u)) != 0) {
                Scheduler::wait(1);
            }
            break;
        case 2:
            // Supported Protocol - dword 0 contains the major/minor version.
            uint8 major = (ext_cap >> 24u) & 0xffu;
            uint8 minor = (ext_cap >> 16u) & 0xffu;
            logln(" usb: Host Controller supports USB {}.{}", major, minor);

            // And dword 2 contains the compatible port offset and count.
            auto port_info = read_cap<uint32>((xecp + 2) * sizeof(uint32));
            uint8 protocol_port_offset = (port_info >> 0u) & 0xffu;
            uint8 protocol_port_count = (port_info >> 8u) & 0xffu;
            for (uint8 count = 0, i = 0; i < protocol_port_count; i++) {
                auto &port = m_ports[protocol_port_offset + i - 1];
                ENSURE(port.major() == 0);
                port.set_major(major);
                port.set_minor(minor);
                port.set_number(protocol_port_offset + i - 1);
                port.set_socket(count++);
            }
            break;
        }
        xecp += (ext_cap >> 8u) & 0xffu;
    } while (((ext_cap >> 8u) & 0xffu) != 0);

    // Pair ports that manage the same socket.
    for (uint8 i = 0; i < port_count; i++) {
        for (uint8 j = 0; j < port_count; j++) {
            auto &a = m_ports[i];
            auto &b = m_ports[j];
            if (i == j || a.socket() != b.socket()) {
                continue;
            }
            a.set_paired(&b);
            b.set_paired(&a);
        }
    }

    uint32 scratch_buffer_count = ((hcs_params2 >> 27u) & 0x1fu) | ((hcs_params2 >> 16u) & 0x3e0u);
    auto *scratchpad = new (ustd::align_val_t(4_KiB)) void *[scratch_buffer_count];
    for (uint32 i = 0; i < scratch_buffer_count; i++) {
        auto *buffer = new (ustd::align_val_t(4_KiB)) uint8[4_KiB];
        memset(buffer, 0, 4_KiB);
        scratchpad[i] = buffer;
    }

    // Array of void * because first element is for scratchpad.
    m_context_table = new (ustd::align_val_t(64_KiB)) void *[slot_count + 1];
    m_context_table[0] = scratchpad;
    write_op(k_op_reg_dcbapp, m_context_table);

    // Allocate the command ring and store it into the Command Ring Control Register.
    m_command_ring = new TrbRing(true);
    write_op(k_op_reg_crcr, reinterpret_cast<uint64>(m_command_ring) | 1u);

    // Set the amount of device slots to be enabled to the maximum.
    write_op<uint32>(k_op_reg_config, slot_count);

    // Set device notification control register.
    write_op<uint32>(k_op_reg_dnctrl, 1u << 1u);

    // Setup event ring and primary interrupter.
    m_event_ring = new TrbRing(false);
    m_interrupter = new Interrupter(m_base + m_run_offset + 0x20, m_event_ring);

    // Clear status bits.
    write_op(k_op_reg_status, (1u << 10u) | (1u << 4u) | (1u << 3u) | (1u << 2u));

    // Start the schedule by setting the run/stop, interrupter enable, and host system error enable bits.
    write_op(k_op_reg_command, (1u << 3u) | (1u << 2u) | (1u << 0u));

    // Wait for startup.
    while ((read_op<uint32>(k_op_reg_status) & 1u) != 0) {
        Scheduler::wait(1);
    }

    // Initialise ports and reset any already-connected ones.
    for (auto &port : m_ports) {
        if (port.major() != 3 || (port.major() == 2 && port.paired() != nullptr)) {
            continue;
        }
        if (port.initialise(this) && port.connected()) {
            if (!port.reset()) {
                logln(" usb: Failed to reset port {}!", port.number());
            }
            continue;
        }
        // If a USB 3 port failed to reset, try to reset USB 2 port in pair instead.
        if (port.major() == 3) {
            auto *paired = port.paired();
            ASSERT(paired != nullptr);
            if (paired->initialise(this) && paired->connected()) {
                if (!paired->reset()) {
                    logln(" usb: Failed to reset port {}!", port.number());
                }
            }
        }
    }
}

void HostController::handle_interrupt() {
    auto status = read_op<uint32>(k_op_reg_status) & ((1u << 10u) | (1u << 4u) | (1u << 3u) | (1u << 2u));
    if (status != 0) {
        write_op(k_op_reg_status, status);
    }

    if (m_interrupter == nullptr || !m_interrupter->interrupt_pending()) {
        return;
    }

    m_interrupter->clear_pending();
    for (auto &event : m_event_ring->dequeue()) {
        const auto event_type = static_cast<uint8>(event.type);
        if (((event.status >> 24u) & 0xffu) != 1) {
            logln(" usb: Received event {} with unsuccessful completion code {}!", event_type,
                  (event.status >> 24u) & 0xffu);
        }
        switch (event.type) {
        case TrbType::TransferEvent: {
            auto *transfer = static_cast<TransferRequestBlock *>(event.data_ptr);
            if (transfer->type == TrbType::Normal) {
                transfer->cycle = !transfer->cycle;
                auto *link = transfer + 1;
                if (link->type == TrbType::Link) {
                    link->cycle = !link->cycle;
                }
            }
            if (transfer->type == TrbType::StatusStage) {
                transfer->status |= (1u << 31u); // Signal status transfer completed
            }
            break;
        }
        case TrbType::CommandCompletionEvent: {
            auto *command = static_cast<TransferRequestBlock *>(event.data_ptr);
            command->data = event.slot;
            command->status = event.status;
            command->status |= (1u << 31u); // Signal command completed
            break;
        }
        case TrbType::PortStatusChangeEvent: {
            auto &port = m_ports[((event.data >> 24u) & 0xffu) - 1];
            if (!port.initialised()) {
                logln(" usb: Uninitialised port {} experienced status change event!", port.number());
                break;
            }
            port.set_status_changed();
            break;
        }
        default:
            logln(" usb: Received unrecognised event {}!", event_type);
            break;
        }
    }
    m_interrupter->acknowledge(m_event_ring->current());

    for (auto &port : m_ports) {
        if (auto *device = port.device()) {
            device->poll();
        }
    }
}

void HostController::on_attach(Port &port) {
    ASSERT(port.connected() && port.device() == nullptr);

    TransferRequestBlock enable_slot{
        .type = TrbType::EnableSlot,
    };
    send_command(&enable_slot);

    // Setup initial identifying device. This device object is only constructed to retrieve the device descriptor. If we
    // end up having a suitable driver for the device, this device gets moved into a heap object. We also pass "this" to
    // the device constructor, which is ok here since all HostController vector reallocation has already been performed
    // at this point.
    const uint8 slot = enable_slot.data & 0xffu;
    usb::Device id_device(this, slot);
    id_device.setup(port);
    m_context_table[slot] = &id_device.slot_context();

    // Put device into default state.
    TransferRequestBlock set_address_first{
        .data_ptr = id_device.context(),
        .block = true,
        .type = TrbType::AddressDevice,
        .slot = slot,
    };
    send_command(&set_address_first);
    ENSURE(id_device.slot_state() == SlotState::Default);

    // Request first 8 bytes of device descriptor.
    Array<uint8, 8> early_device_descriptor{};
    id_device.read_descriptor(early_device_descriptor.span());

    // Reset port again.
    if (!port.reset()) {
        ENSURE_NOT_REACHED("Failed to reset port!");
    }

    // Put device into addressed state.
    TransferRequestBlock set_address_second{
        .data_ptr = id_device.context(),
        .type = TrbType::AddressDevice,
        .slot = slot,
    };
    send_command(&set_address_second);
    ENSURE(id_device.slot_state() == SlotState::Addressed);

    // Retrieve actual device descriptor length from first 8 bytes.
    const auto device_descriptor_length = early_device_descriptor[0];
    ENSURE(device_descriptor_length >= sizeof(DeviceDescriptor));

    Vector<uint8> device_descriptor_bytes(device_descriptor_length);
    id_device.read_descriptor(device_descriptor_bytes.span());

    auto *device_descriptor = reinterpret_cast<DeviceDescriptor *>(device_descriptor_bytes.data());
    auto *device = UsbManager::register_device(ustd::move(id_device), device_descriptor);
    port.set_device(device);
}

void HostController::on_detach(Port &port) {
    auto *device = port.device();
    if (device == nullptr) {
        // Device may be null if we don't have a suitable driver for it.
        return;
    }

    device->disconnect();
    TransferRequestBlock disable_slot{
        .type = TrbType::DisableSlot,
        .slot = device->slot(),
    };
    send_command(&disable_slot);
    port.set_device(nullptr);
}

void HostController::send_command(TransferRequestBlock *command) {
    auto *trb = m_command_ring->enqueue(command);
    ring_doorbell(0, 0);
    while ((trb->status & (1u << 31u)) == 0) {
        Scheduler::wait(1);
    }
    memcpy(command, trb, sizeof(TransferRequestBlock));
}

void HostController::spawn_watch_thread() {
    auto *watch_thread = Thread::create_kernel(&watch_loop);
    watch_thread->register_state().rdi = reinterpret_cast<uintptr>(this);
    Scheduler::insert_thread(watch_thread);
}

} // namespace usb
