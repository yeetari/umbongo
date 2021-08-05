#include <core/Error.hh>

#include <ustd/Array.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {
namespace {

// These need to match with the enum in kernel/SysError.hh
// clang-format off
Array k_error_list{
    "success",
    "bad fd",
    "no such file or directory",
    "broken handle",
    "invalid",
};
// clang-format on

} // namespace

StringView error_string(ssize rc) {
    rc = -rc;
    if (rc < 0 || static_cast<usize>(rc) >= k_error_list.size()) {
        return "Unknown error"sv;
    }
    return k_error_list[static_cast<usize>(rc)];
}

} // namespace core
