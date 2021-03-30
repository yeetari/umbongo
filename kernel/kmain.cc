#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/PciTable.hh>
#include <kernel/acpi/RootTable.hh>
#include <kernel/acpi/RootTablePtr.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/pci/Bus.hh>
#include <kernel/pci/Device.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <libelf/Elf.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

InterruptPolarity interrupt_polarity(uint8 polarity) {
    switch (polarity) {
    case 0b00:
    case 0b01:
        return InterruptPolarity::ActiveHigh;
    case 0b11:
        return InterruptPolarity::ActiveLow;
    default:
        ENSURE_NOT_REACHED();
    }
}

InterruptTriggerMode interrupt_trigger_mode(uint8 trigger_mode) {
    switch (trigger_mode) {
    case 0b00:
    case 0b01:
        return InterruptTriggerMode::EdgeTriggered;
    case 0b11:
        return InterruptTriggerMode::LevelTriggered;
    default:
        ENSURE_NOT_REACHED();
    }
}

void kernel_init(BootInfo *boot_info, acpi::RootTable *xsdt) {
    auto *mcfg = xsdt->find<acpi::PciTable>();
    ENSURE(mcfg != nullptr);
    for (const auto *segment : *mcfg) {
        logln("acpi: Found PCI segment {} at {:h} (start_bus={}, end_bus={})", segment->num, segment->base,
              segment->start_bus, segment->end_bus);
    }

    struct PciDevice {
        pci::Bus *bus;
        uint8 device;
        uint8 function;
        uint16 vendor_id;
        uint16 device_id;
        uint8 clas;
        uint8 subc;
        uint8 prif;
    };

    // Enumerate all PCI devices.
    Vector<PciDevice> pci_devices;
    for (const auto *segment : *mcfg) {
        const uint8 bus_count = segment->end_bus - segment->start_bus;
        for (uint8 bus_num = 0; bus_num < bus_count; bus_num++) {
            auto *bus = new pci::Bus(segment->base, segment->num, bus_num);
            for (uint8 device_num = 0; device_num < 32; device_num++) {
                for (uint8 function = 0; function < 8; function++) {
                    pci::Device device(bus, device_num, function);
                    if (device.vendor_id() == 0xffffu) {
                        continue;
                    }
                    auto class_info = device.class_info();
                    uint8 clas = (class_info >> 24u) & 0xffu;
                    uint8 subc = (class_info >> 16u) & 0xffu;
                    uint8 prif = (class_info >> 8u) & 0xffu;
                    pci_devices.push(
                        {bus, device_num, function, device.vendor_id(), device.device_id(), clas, subc, prif});
                }
            }
        }
    }

    // List all found PCI devices.
    logln(" pci: Found {} devices total", pci_devices.size());
    logln(" pci:  - SEGM   BUS  DEV  FUNC   VENDID DEVID   CLAS SUBC PRIF");
    for (auto &device : pci_devices) {
        logln(" pci:  - {:h4}:{:h2}:{:h2}:{:h2} - {:h4}:{:h4} ({:h2}:{:h2}:{:h2})", device.bus->segment_num(),
              device.bus->num(), device.device, device.function, device.vendor_id, device.device_id, device.clas,
              device.subc, device.prif);
    }

    RamFsEntry *init_entry = nullptr;
    for (auto *entry = boot_info->ram_fs; entry != nullptr; entry = entry->next) {
        if (strcmp(entry->name, "init") == 0) {
            ASSERT(init_entry == nullptr);
            init_entry = entry;
        }
        logln("  fs: Found file {}", entry->name);
    }
    ENSURE(init_entry != nullptr, "Failed to find init program!");

    const auto *header = reinterpret_cast<const elf::Header *>(init_entry->data);
    usize mem_size = 0;
    for (uint16 i = 0; i < header->ph_count; i++) {
        const auto *phdr = reinterpret_cast<const elf::ProgramHeader *>(
            reinterpret_cast<uintptr>(header) + static_cast<uintptr>(header->ph_off) + header->ph_size * i);
        if (phdr->type == elf::ProgramHeaderType::Load) {
            mem_size += phdr->memsz;
        }
    }

    auto *data = new (std::align_val_t(4_KiB)) uint8[mem_size];
    for (uint16 i = 0; i < header->ph_count; i++) {
        const auto *phdr = reinterpret_cast<const elf::ProgramHeader *>(
            reinterpret_cast<uintptr>(header) + static_cast<uintptr>(header->ph_off) + header->ph_size * i);
        if (phdr->type != elf::ProgramHeaderType::Load) {
            continue;
        }
        ASSERT(phdr->filesz <= phdr->memsz);
        memcpy(data + phdr->vaddr, init_entry->data + phdr->offset, phdr->filesz);
    }

    // Spawn 5 identical user processes.
    for (int i = 0; i < 5; i++) {
        auto *init_process = Process::create_user(data, mem_size);
        init_process->set_entry_point(header->entry);
        Scheduler::insert_process(init_process);
    }

    // TODO: Kill current process and yield.
    while (true) {
        asm volatile("hlt");
    }
}

} // namespace

usize __stack_chk_guard = 0xdeadc0de;

[[noreturn]] extern "C" void __stack_chk_fail() {
    auto *rbp = static_cast<uint64 *>(__builtin_frame_address(0));
    while (rbp != nullptr && rbp[1] != 0) {
        logln("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

void put_char(char ch) {
    Console::put_char(ch);
    if constexpr (k_kernel_qemu_debug) {
        port_write(0xe9, ch);
    }
}

extern "C" void kmain(BootInfo *boot_info) {
    Console::initialise(boot_info);
    logln("core: Using font {} {}", g_font.name(), g_font.style());
    if constexpr (k_kernel_qemu_debug) {
        ENSURE(port_read(0xe9) == 0xe9, "KERNEL_QEMU_DEBUG config option enabled, but port e9 isn't available!");
    }
    if constexpr (k_kernel_stack_protector) {
        logln("core: SSP initialised with guard value {:h}", __stack_chk_guard);
    }

    logln("core: boot_info = {}", boot_info);
    logln("core: framebuffer = {:h} ({}x{})", boot_info->framebuffer_base, boot_info->width, boot_info->height);
    logln("core: rsdp = {}", boot_info->rsdp);

    MemoryManager::initialise(boot_info);
    Processor::initialise();

    auto *rsdp = reinterpret_cast<acpi::RootTablePtr *>(boot_info->rsdp);
    ENSURE(rsdp->revision() == 2, "ACPI 2.0+ required!");

    auto *xsdt = rsdp->xsdt();
    logln("acpi: XSDT = {} (revision={}, valid={})", xsdt, xsdt->revision(), xsdt->valid());
    ENSURE(xsdt->valid());

    for (auto *entry : *xsdt) {
        logln("acpi:  - {:c}{:c}{:c}{:c} = {} (revision={}, valid={})", entry->signature()[0], entry->signature()[1],
              entry->signature()[2], entry->signature()[3], entry, entry->revision(), entry->valid());
        ENSURE(entry->valid());
    }

    auto *madt = xsdt->find<acpi::ApicTable>();
    ENSURE(madt != nullptr);
    if ((madt->flags() & 1u) != 0) {
        InterruptManager::mask_pic();
    }

    for (auto *controller : *madt) {
        // TODO: Handle local APIC address override.
        ENSURE(controller->type != 5);
        if (controller->type == 1) {
            auto *io_apic = reinterpret_cast<acpi::IoApicController *>(controller);
            InterruptManager::register_io_apic(io_apic->address, io_apic->gsi_base);
        }
        if (controller->type == 2) {
            auto *override = reinterpret_cast<acpi::InterruptSourceOverride *>(controller);
            ENSURE(override->bus == 0);
            InterruptManager::register_override(override->isa, override->gsi, interrupt_polarity(override->polarity),
                                                interrupt_trigger_mode(override->trigger_mode));
        }
    }

    // Enable local APIC and acknowledge any outstanding interrupts.
    auto *apic = reinterpret_cast<LocalApic *>(madt->local_apic());
    logln("acpi: Local APIC = {}", apic);
    apic->enable();
    apic->send_eoi();
    Processor::set_apic(apic);

    auto *kernel_init_process = Process::create_kernel();
    kernel_init_process->set_entry_point(reinterpret_cast<uintptr>(&kernel_init));
    kernel_init_process->register_state().rdi = reinterpret_cast<uintptr>(boot_info);
    kernel_init_process->register_state().rsi = reinterpret_cast<uintptr>(xsdt);

    Scheduler::initialise();
    Scheduler::insert_process(kernel_init_process);
    Scheduler::start();
}
