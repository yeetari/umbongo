#include <log/level.hh>
#include <log/log.hh>
#include <system/syscall.hh>
#include <ustd/assert.hh>
#include <ustd/string.hh>
#include <ustd/string_builder.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

size_t main(size_t argc, const char **argv);

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    // TODO: Avoid allocating memory here.
    ustd::StringBuilder builder;
    builder.append("Assertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        builder.append("\n=> {}", msg);
    }
    log::message(log::Level::Error, builder.build());
    static_cast<void>(system::syscall(UB_SYS_exit, 1));
    while (true) {
        asm volatile("ud2");
    }
}

extern "C" void _start(size_t argc, const char **argv) {
    asm volatile("andq $~15, %rsp");
    EXPECT(system::syscall(UB_SYS_exit, main(argc, argv)));
}
