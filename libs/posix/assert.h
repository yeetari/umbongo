#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

#ifndef NDEBUG
#define assert(expr) __builtin_expect((expr), 0) ? (void)0 : __assertion_failed(__FILE__, __LINE__, #expr)
#else
#define assert(expr) ((void)0)
#endif

__attribute__((noreturn)) void __assertion_failed(const char *file, unsigned int line, const char *expr);

__END_DECLS
