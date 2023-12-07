#pragma once

#include <system/Error.h>
#include <system/System.h>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<void, ub_error_t> chdir(const char *path);
ustd::Result<size_t, ub_error_t> create_process(const char *path);
ustd::Result<size_t, ub_error_t> create_process(const char *path, ustd::Vector<const char *> argv);
ustd::Result<size_t, ub_error_t> create_process(const char *path, ustd::Vector<const char *> argv,
                                                ustd::Vector<ub_fd_pair_t> copy_fds);
[[noreturn]] void exit(size_t code);

ustd::String cwd();
size_t pid();
ustd::Result<void, ub_error_t> wait_pid(size_t pid);

} // namespace core
