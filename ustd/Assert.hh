#pragma once

// TODO: Config options.
#ifndef NDEBUG
#define ASSERTIONS
#define ASSERTIONS_PEDANTIC
#endif

#define ENSURE(expr, msg)                                                                                              \
    static_cast<bool>(expr) ? static_cast<void>(0) : assertion_failed(__FILE__, __LINE__, #expr, msg)
#ifdef ASSERTIONS
#define ASSERT(expr, msg) ENSURE(expr, msg)
#else
#define ASSERT(expr, msg)
#endif
#ifdef ASSERTIONS_PEDANTIC
#define ASSERT_PEDANTIC(expr, msg) ENSURE(expr, msg)
#else
#define ASSERT_PEDANTIC(expr, msg)
#endif

#define ASSERT_NOT_REACHED(msg) ASSERT(false, msg)
#define ENSURE_NOT_REACHED(msg) ENSURE(false, msg)

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg);
