#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
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

extern "C" void jump_to_user(void *stack, void (*callee)());

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

    MemoryManager memory_manager(boot_info);
    Processor::initialise();

    RamFsEntry *init_entry = nullptr;
    for (auto *entry = boot_info->ram_fs; entry != nullptr; entry = entry->next) {
        if (strcmp(entry->name, "init") == 0) {
            ASSERT(init_entry == nullptr);
            init_entry = entry;
        }
        logln("  fs: Found file {}", entry->name);
    }
    ENSURE(init_entry != nullptr, "Failed to find init program!");

    VirtSpace virt_space;
    virt_space.map_4KiB(515_GiB, reinterpret_cast<uintptr>(memory_manager.alloc_phys(4_KiB)),
                        PageFlags::Writable | PageFlags::User | PageFlags::NoExecute);

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

    constexpr uintptr load_addr = 516_GiB;
    for (uintptr addr = 0; addr <= round_up(mem_size, 4_KiB); addr += 4_KiB) {
        virt_space.map_4KiB(load_addr + addr, reinterpret_cast<uintptr>(data) + addr,
                            PageFlags::Writable | PageFlags::User);
    }

    virt_space.switch_to();
    jump_to_user(reinterpret_cast<void *>(515_GiB + 4_KiB), reinterpret_cast<void (*)()>(load_addr + header->entry));
}
