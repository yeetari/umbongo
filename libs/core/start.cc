#include <core/Syscall.hh>
#include <log/Level.hh>
#include <log/Log.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv);

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

extern "C" void _start(usize argc, const char **argv) {
    asm volatile("andq $~15, %rsp");
    EXPECT(core::syscall(Syscall::exit, main(argc, argv)));
}
