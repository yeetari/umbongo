#include <core/Error.hh>

#include <core/Syscall.hh>
#include <log/Log.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {
namespace {

// These need to match with the enum in kernel/SysError.hh
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

[[noreturn]] void abort_error(ustd::StringView msg, SysError error) {
    log::error("{}: {}", msg, error_string(error));
    EXPECT(syscall(Syscall::exit, 1));
    ENSURE_NOT_REACHED();
}

ustd::StringView error_string(SysError error) {
    ssize num = -static_cast<ssize>(error);
    if (num < 0 || static_cast<usize>(num) >= k_error_list.size()) {
        return "unknown error"sv;
    }
    return k_error_list[static_cast<usize>(num)];
}

} // namespace core
