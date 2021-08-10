#pragma once

typedef __builtin_va_list va_list;

#define va_start(ap, n) __builtin_va_start(ap, n)
#define va_copy(dst, src) __builtin_va_copy(dst, src)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)
