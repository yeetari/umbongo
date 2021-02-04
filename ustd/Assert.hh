#pragma once

#include <ustd/Log.hh>

// TODO: Config options.
#ifndef NDEBUG
#define ASSERTIONS
#define ASSERTIONS_PEDANTIC
#endif

#define ENSURE(expr, ...)                                                                                              \
    static_cast<bool>(expr) ? static_cast<void>(0) : assertion_failed(__FILE__, __LINE__, #expr, ##__VA_ARGS__)
#ifdef ASSERTIONS
#define ASSERT(expr, ...) ENSURE(expr, ##__VA_ARGS__)
#else
#define ASSERT(expr, ...)
#endif
#ifdef ASSERTIONS_PEDANTIC
#define ASSERT_PEDANTIC(expr, ...) ENSURE(expr, ##__VA_ARGS__)
#else
#define ASSERT_PEDANTIC(expr, ...)
#endif

#define ASSERT_NOT_REACHED(...) ASSERT(false, ##__VA_ARGS__)
#define ENSURE_NOT_REACHED(...) ENSURE(false, ##__VA_ARGS__)

template <typename... Args>
[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, Args... args) {
    log("Assertion '{}' failed at {}:{}", expr, file, line);
    (log("=> {}", args), ...);
#if defined(BOOTLOADER) || defined(KERNEL)
    while (true) {
        asm volatile("cli");
        asm volatile("hlt");
    }
#else
    // TODO: Abort in userland.
    while (true) {
        log("Assertion in userland!");
    }
#endif
}
