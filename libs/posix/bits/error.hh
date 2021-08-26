#pragma once

#include <errno.h>
#include <sys/types.h>

#include <kernel/SysError.hh>
#include <ustd/Assert.hh>

namespace posix {

inline int to_errno(ssize_t error) {
    switch (static_cast<SysError>(error)) {
    case SysError::BadFd:
        return EBADF;
    case SysError::NonExistent:
        return ENOENT;
    case SysError::BrokenHandle:
        return EIO;
    case SysError::Invalid:
        return EINVAL;
    case SysError::NoExec:
        return ENOEXEC;
    case SysError::NotDirectory:
        return ENOTDIR;
    case SysError::AlreadyExists:
        return EEXIST;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace posix
