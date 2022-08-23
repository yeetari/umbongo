#pragma once

#define ENSURE(expr, ...)                                                                                              \
    static_cast<bool>(expr) ? static_cast<void>(0)                                                                     \
                            : assertion_failed(__FILE__, __LINE__, #expr __VA_OPT__(, ) __VA_ARGS__)

#ifdef ASSERTIONS
#define ASSERT(expr, ...) ENSURE(expr __VA_OPT__(, ) __VA_ARGS__)
#else
#define ASSERT(expr, ...)
#endif
#ifdef ASSERTIONS_PEDANTIC
#define ASSERT_PEDANTIC(expr, ...) ENSURE(expr __VA_OPT__(, ) __VA_ARGS__)
#else
#define ASSERT_PEDANTIC(expr, ...)
#endif

#define ASSERT_NOT_REACHED(...) ASSERT(false __VA_OPT__(, ) __VA_ARGS__)
#define ENSURE_NOT_REACHED(...) ENSURE(false __VA_OPT__(, ) __VA_ARGS__)

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg = nullptr);
