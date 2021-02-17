#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/mem/MemoryManager.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

// NOLINTNEXTLINE
usize __stack_chk_guard = 0xdeadc0de;

// NOLINTNEXTLINE
[[noreturn]] extern "C" void __stack_chk_fail() {
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

void put_char(char ch) {
    Console::put_char(ch);
    if constexpr (k_kernel_qemu_debug) {
        port_write(0xe9, ch);
    }
}

void handle_interrupt(InterruptFrame *frame) {
    logln("core: Received interrupt {}", frame->num);
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
    logln("core: framebuffer = {:h}", boot_info->framebuffer_base);

    MemoryManager memory_manager(boot_info);
    Processor::initialise(memory_manager);
    Processor::wire_interrupt(0, &handle_interrupt);
    Processor::wire_interrupt(31, &handle_interrupt);
    Processor::wire_interrupt(255, &handle_interrupt);
    asm volatile("int $0");
    asm volatile("int $31");
    asm volatile("int $255");
}
