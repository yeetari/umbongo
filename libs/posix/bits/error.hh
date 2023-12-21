#pragma once

#include <errno.h>

#include <system/error.h>
#include <ustd/assert.hh>

namespace posix {

inline int to_errno(ub_error_t error) {
    switch (error) {
    case UB_ERROR_BAD_FD:
        return EBADF;
    case UB_ERROR_NON_EXISTENT:
        return ENOENT;
    case UB_ERROR_BROKEN_HANDLE:
        return EIO;
    case UB_ERROR_INVALID:
        return EINVAL;
    case UB_ERROR_NO_EXEC:
        return ENOEXEC;
    case UB_ERROR_NOT_DIRECTORY:
        return ENOTDIR;
    case UB_ERROR_ALREADY_EXISTS:
        return EEXIST;
    case UB_ERROR_BUSY:
        return EBUSY;
    default:
        ENSURE_NOT_REACHED();
    }
}

} // namespace posix
