#include <core/Error.hh>

#include <core/Debug.hh>
#include <system/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Format.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {
namespace {

// These need to match with the values in kernel/api/Errors.in
// clang-format off
ustd::Array k_error_list{
    "success"sv,
    "bad access",
    "bad address",
    "bad handle",
    "too big",
    "virt space full",
    "no exec",
};
// clang-format on

} // namespace

[[noreturn]] void abort_error(ustd::StringView msg, ub_error_t error) {
    core::debug_line("{}: {}", msg, error_string(error));
    EXPECT(system::syscall(SYS_proc_exit, 1));
    __builtin_unreachable();
}

ustd::StringView error_string(ub_error_t error) {
    ssize_t index = -static_cast<ssize_t>(error);
    if (index < 0 || static_cast<size_t>(index) >= k_error_list.size()) {
        return "unknown error"sv;
    }
    return k_error_list[static_cast<size_t>(index)];
}

} // namespace core
