#include <core/Syscall.hh>
#include <log/Level.hh>
#include <log/Log.hh>
#include <ustd/Assert.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

size_t main(size_t argc, const char **argv);

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    // TODO: Avoid allocating memory here.
    ustd::StringBuilder builder;
    builder.append("Assertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        builder.append("\n=> {}", msg);
    }
    log::message(log::Level::Error, builder.build());
    static_cast<void>(core::syscall(Syscall::exit, 1));
    while (true) {
        asm volatile("ud2");
    }
}

extern "C" void _start(size_t argc, const char **argv) {
    asm volatile("andq $~15, %rsp");
    EXPECT(core::syscall(Syscall::exit, main(argc, argv)));
}
