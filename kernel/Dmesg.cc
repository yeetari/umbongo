#include <kernel/Dmesg.hh>

#include <kernel/Console.hh>
#include <kernel/SpinLock.hh>
#include <kernel/dev/DmesgDevice.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>

#ifdef KERNEL_QEMU_DEBUG
#include <kernel/Port.hh>
#endif

namespace kernel {
namespace {

SpinLock s_lock;

} // namespace

bool g_console_enabled = true;

void dmesg_lock() {
    s_lock.lock();
}

void dmesg_unlock() {
    s_lock.unlock();
}

void dmesg_put_char(char ch) {
    DmesgDevice::put_char(ch);
#ifdef KERNEL_QEMU_DEBUG
    port_write(0xe9, ch);
#endif
    if (g_console_enabled) {
        Console::put_char(ch);
    }
}

void dmesg_single(const char *, bool arg) {
    dmesg_single("", arg ? "true" : "false");
}

void dmesg_single(const char *, const char *arg) {
    while (*arg != '\0') {
        dmesg_put_char(*arg++);
    }
}

void dmesg_single(const char *, ustd::StringView arg) {
    for (char ch : arg) {
        dmesg_put_char(ch);
    }
}

void dmesg_single(const char *, const ustd::String &arg) {
    for (char ch : arg) {
        dmesg_put_char(ch);
    }
}

} // namespace kernel
