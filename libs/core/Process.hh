#pragma once

#include <core/Error.hh>
#include <kernel/SyscallTypes.hh> // IWYU pragma: keep
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<void, SysError> chdir(const char *path);
ustd::Result<size_t, SysError> create_process(const char *path);
ustd::Result<size_t, SysError> create_process(const char *path, ustd::Vector<const char *> argv);
ustd::Result<size_t, SysError> create_process(const char *path, ustd::Vector<const char *> argv,
                                              ustd::Vector<kernel::FdPair> copy_fds);
[[noreturn]] void exit(size_t code);

ustd::String cwd();
size_t pid();
ustd::Result<void, SysError> wait_pid(size_t pid);

} // namespace core
