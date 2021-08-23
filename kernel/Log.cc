#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Port.hh>
#include <kernel/SpinLock.hh>

bool g_console_enabled = true;

namespace {

SpinLock s_lock;

} // namespace

void dbg_put_char(char ch) {
    if constexpr (k_kernel_qemu_debug) {
        port_write(0xe9, ch);
    }
    if (g_console_enabled) {
        Console::put_char(ch);
    }
}

void log_put_char(char ch) {
    dbg_put_char(ch);
}

void log_lock() {
    s_lock.lock();
}

void log_unlock() {
    s_lock.unlock();
}