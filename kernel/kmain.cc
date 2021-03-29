#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/RootTable.hh>
#include <kernel/acpi/RootTablePtr.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <libelf/Elf.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

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
    if ((madt->flags() & (1u << 0u)) != 0) {
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

    Scheduler::initialise(apic);
    for (int i = 0; i < 5; i++) {
        auto *init_process = Process::create_user(data, mem_size);
        init_process->set_entry_point(header->entry);
        Scheduler::insert_process(init_process);
    }
    Scheduler::start();
}
