#pragma once

#if '\xff' > 0
#error
#endif

#define CHAR_BIT 8
#define CHAR_MAX SCHAR_MAX
#define CHAR_MIN SCHAR_MIN

#define SHRT_BIT 16
#define SHRT_MAX 0x7fff
#define SHRT_MIN (-SHRT_MAX - 1)

#define INT_BIT 32
#define INT_MAX 0x7fffffff
#define INT_MIN (-INT_MAX - 1)

#define LONG_BIT 64
#define LONG_MAX 0x7fffffffffffffffl
#define LONG_MIN (-LONG_MAX - 1)

#define LLONG_MAX 0x7fffffffffffffffll
#define LLONG_MIN (-LLONG_MAX - 1)

#define SCHAR_MAX 127
#define SCHAR_MIN (-128)
#define UCHAR_MAX 0xffu
#define USHRT_MAX 0xffffu
#define UINT_MAX 0xffffffffu
#define ULONG_MAX (2ul * LONG_MAX + 1)
#define ULLONG_MAX (2ull * LLONG_MAX + 1)

#define SSIZE_MAX LONG_MAX
#define WORD_BIT INT_BIT
