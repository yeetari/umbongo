#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

extern uint8 k_user_code_start;
extern uint8 k_user_code_end;

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

    auto *user_stack = static_cast<char *>(memory_manager.alloc_phys(4_KiB));

    VirtSpace virt_space;
    virt_space.map_4KiB(514_GiB, reinterpret_cast<uintptr>(&k_user_code_start), PageFlags::User);
    virt_space.map_4KiB(515_GiB, reinterpret_cast<uintptr>(user_stack),
                        PageFlags::Writable | PageFlags::User | PageFlags::NoExecute);
    virt_space.switch_to();
    jump_to_user(reinterpret_cast<void *>(515_GiB + 4_KiB), reinterpret_cast<void (*)()>(514_GiB));
}
