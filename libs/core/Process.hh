#pragma once

#include <core/Error.hh>
#include <kernel/SyscallTypes.hh> // IWYU pragma: keep
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<usize, SysError> create_process(const char *path);
ustd::Result<usize, SysError> create_process(const char *path, ustd::Vector<const char *> argv);
ustd::Result<usize, SysError> create_process(const char *path, ustd::Vector<const char *> argv,
                                             ustd::Vector<kernel::FdPair> copy_fds);
[[noreturn]] void exit(usize code);

} // namespace core
