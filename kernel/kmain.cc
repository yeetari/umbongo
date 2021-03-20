#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/RootTable.hh>
#include <kernel/acpi/RootTablePtr.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/VirtSpace.hh>
#include <libelf/Elf.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

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

extern "C" void jump_to_user(void (*entry)(), void *stack);

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
        logln("acpi: Found legacy PIC, masking");
        port_write<uint8>(0x21, 0xff);
        port_write<uint8>(0xa1, 0xff);
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

    auto *data = new uint8[mem_size];
    for (uint16 i = 0; i < header->ph_count; i++) {
        const auto *phdr = reinterpret_cast<const elf::ProgramHeader *>(
            reinterpret_cast<uintptr>(header) + static_cast<uintptr>(header->ph_off) + header->ph_size * i);
        if (phdr->type != elf::ProgramHeaderType::Load) {
            continue;
        }
        ASSERT(phdr->filesz <= phdr->memsz);
        memcpy(data + phdr->vaddr, init_entry->data + phdr->offset, phdr->filesz);
    }

    VirtSpace virt_space = VirtSpace::create_user(data, mem_size);
    virt_space.switch_to();
    jump_to_user(reinterpret_cast<void (*)()>(k_user_binary_base + header->entry),
                 reinterpret_cast<void *>(k_user_stack_head));
}
