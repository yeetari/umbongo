#include <core/Debug.hh>
#include <system/Syscall.hh>
#include <ustd/Assert.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

size_t main();

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    // TODO: Avoid allocating memory here.
    ustd::StringBuilder builder;
    builder.append("Assertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        builder.append("\n=> {}", msg);
    }
    core::debug_line(builder.build());
    static_cast<void>(system::syscall(SYS_proc_exit, 1));
    asm volatile("ud2");
    __builtin_unreachable();
}

extern "C" void _start() {
    asm volatile("andq $~15, %rsp");
    EXPECT(system::syscall(SYS_proc_exit, main()));
}
