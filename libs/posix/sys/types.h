#pragma once

#include <stddef.h>
#include <stdint.h>

typedef uint32_t ino_t;
typedef int64_t off_t;

typedef uint32_t blkcnt_t;
typedef uint32_t blksize_t;
typedef uint32_t dev_t;
typedef uint16_t mode_t;
typedef uint32_t nlink_t;

typedef uint32_t uid_t;
typedef uint32_t gid_t;

typedef int32_t suseconds_t;
typedef int64_t time_t;

struct utimbuf {
    time_t actime;
    time_t modtime;
};

typedef int __pid_t;
#define pid_t __pid_t

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wkeyword-macro"
#define unsigned signed
typedef __SIZE_TYPE__ ssize_t;
#undef unsigned
#pragma clang diagnostic pop
