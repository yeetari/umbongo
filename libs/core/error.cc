#include <core/error.hh>

#include <log/log.hh>
#include <system/error.h>
#include <system/syscall.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

namespace core {
namespace {

// These need to match with the order in kernel/api/Errors.in
// clang-format off
ustd::Array k_error_list{
    "success",
    "bad fd",
    "no such file or directory",
    "broken handle",
    "invalid",
    "exec format error",
    "not a directory",
    "already exists",
    "resource busy",
};
// clang-format on

} // namespace

[[noreturn]] void abort_error(ustd::StringView msg, ub_error_t error) {
    log::error("{}: {}", msg, error_string(error));
    EXPECT(system::syscall(UB_SYS_exit, 1));
    ENSURE_NOT_REACHED();
}

ustd::StringView error_string(ub_error_t error) {
    ssize_t num = -static_cast<ssize_t>(error);
    if (num < 0 || static_cast<size_t>(num) >= k_error_list.size()) {
        return "unknown error"sv;
    }
    return k_error_list[static_cast<size_t>(num)];
}

} // namespace core
