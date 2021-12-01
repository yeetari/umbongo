#include <core/Error.hh>

#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
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

[[noreturn]] void abort_error(ustd::StringView msg, ssize rc) {
    dbgln("{}: {}", msg, error_string(rc));
    Syscall::invoke(Syscall::exit, 1);
    ENSURE_NOT_REACHED();
}

ustd::StringView error_string(ssize rc) {
    rc = -rc;
    if (rc < 0 || static_cast<usize>(rc) >= k_error_list.size()) {
        return "unknown error"sv;
    }
    return k_error_list[static_cast<usize>(rc)];
}

} // namespace core
