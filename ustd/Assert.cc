#include <ustd/Assert.hh>

#include <ustd/Log.hh>

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    logln("\nAssertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        logln("=> {}", msg);
    }
    while (true) {
        asm volatile("cli; hlt");
    }
}
