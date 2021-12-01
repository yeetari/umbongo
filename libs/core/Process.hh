#pragma once

#include <kernel/SyscallTypes.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ssize create_process(const char *path);
ssize create_process(const char *path, ustd::Vector<const char *> argv);
ssize create_process(const char *path, ustd::Vector<const char *> argv, ustd::Vector<FdPair> copy_fds);

} // namespace core
