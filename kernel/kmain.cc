#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/HpetTable.hh>
#include <kernel/acpi/InterruptController.hh>
#include <kernel/acpi/PciTable.hh>
#include <kernel/acpi/RootTable.hh>
#include <kernel/acpi/RootTablePtr.hh>
#include <kernel/acpi/Table.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Paging.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/RamFs.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/intr/InterruptType.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/pci/Bus.hh>
#include <kernel/pci/Device.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/usb/UsbManager.hh>
#include <libelf/Elf.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
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

    // Attempt to initialise any found XHCI controllers.
    for (auto &device : pci_devices) {
        if (device.clas == 0x0c && device.subc == 0x03 && device.prif == 0x30) {
            usb::UsbManager::register_host_controller(device.bus, device.device, device.function);
        }
    }
    usb::UsbManager::spawn_watch_threads();

    // Setup in-memory file system.
    auto root_fs = ustd::make_unique<RamFs>();
    Vfs::initialise();
    Vfs::mount_root(ustd::move(root_fs));

    // Copy over files loaded by UEFI into the ramdisk.
    for (auto *entry = boot_info->ram_fs; entry != nullptr; entry = entry->next) {
        if (entry->is_directory) {
            Vfs::mkdir(entry->name);
            continue;
        }
        auto file = Vfs::create(entry->name);
        file->write({entry->data, entry->data_size});
    }

    // TODO: Tell the MemoryManager to make available reclaimable memory too. Once LoaderData is marked as reclaimable
    //       in the bootloader, we can reclaim the initial ramfs data after copying it over.

    auto init_file = Vfs::open("/init");
    auto *virt_space = VirtSpace::create_user();
    virt_space->switch_to();

    elf::Header header{};
    init_file->read({&header, sizeof(elf::Header)});
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        init_file->read({&phdr, sizeof(elf::ProgramHeader)}, static_cast<usize>(header.ph_off + header.ph_size * i));
        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }
        ASSERT(phdr.filesz <= phdr.memsz);
        uintptr segment_start = round_down(k_user_binary_base + phdr.vaddr, 4_KiB);
        uintptr segment_end = round_up(k_user_binary_base + phdr.vaddr + phdr.memsz, 4_KiB);
        for (uintptr page = segment_start; page < segment_end; page += 4_KiB) {
            // TODO: Don't always map writable. Also map NoExecute where possible.
            auto frame = reinterpret_cast<uintptr>(MemoryManager::instance().alloc_phys(4_KiB));
            virt_space->map_4KiB(page, frame, PageFlags::Writable | PageFlags::User);
        }

        // Force flush TLB to get the new mappings so we can memcpy the data in.
        virt_space->switch_to();

        usize copy_offset = phdr.vaddr & 0xfffu;
        init_file->read({reinterpret_cast<void *>(segment_start + copy_offset), phdr.memsz},
                        static_cast<usize>(phdr.offset));
    }

    auto *init_process = Process::create_user(virt_space);
    init_process->set_entry_point(header.entry);
    Scheduler::insert_process(init_process);

    // TODO: Kill current process and yield.
    while (true) {
        asm volatile("hlt");
    }
}

} // namespace

extern void (*k_ctors_start)();
extern void (*k_ctors_end)();

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

    // Invoke global constructors.
    logln("core: Invoking {} global constructors", &k_ctors_end - &k_ctors_start);
    for (void (**ctor)() = &k_ctors_start; ctor < &k_ctors_end; ctor++) {
        (*ctor)();
    }

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

    auto *hpet_table = xsdt->find<acpi::HpetTable>();
    ENSURE(hpet_table != nullptr);

    logln("core: Creating init process");
    auto *kernel_init_process = Process::create_kernel();
    kernel_init_process->set_entry_point(reinterpret_cast<uintptr>(&kernel_init));
    kernel_init_process->register_state().rdi = reinterpret_cast<uintptr>(boot_info);
    kernel_init_process->register_state().rsi = reinterpret_cast<uintptr>(xsdt);

    Scheduler::initialise(hpet_table);
    Scheduler::insert_process(kernel_init_process);
    Scheduler::start();
}

extern "C" int __cxa_atexit(void (*)(void *), void *, void *) {
    return 0;
}

[[noreturn]] extern "C" void __cxa_pure_virtual() {
    ENSURE_NOT_REACHED("__cxa_pure_virtual");
}
