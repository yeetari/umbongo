#pragma once

#define NULL ((void *) 0)
#define offsetof(type, member) __builtin_offsetof(type, member)

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __SIZE_TYPE__ size_t;
