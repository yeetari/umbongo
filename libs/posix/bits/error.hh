#pragma once

#include <errno.h>

#include <core/Error.hh>
#include <ustd/Assert.hh>

namespace posix {

inline int to_errno(core::SysError error) {
    using core::SysError;
    switch (error) {
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
    case SysError::Busy:
        return EBUSY;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace posix
