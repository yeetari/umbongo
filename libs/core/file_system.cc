#include <core/file_system.hh>

#include <system/error.h>
#include <system/syscall.hh>
#include <ustd/result.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>

namespace core {

ustd::Result<void, ub_error_t> mount(ustd::StringView target, ustd::StringView fs_type) {
    if (auto result = system::syscall(UB_SYS_mkdir, target.data());
        result.is_error() && result.error() != UB_ERROR_ALREADY_EXISTS) {
        return result.error();
    }
    TRY(system::syscall(UB_SYS_mount, target.data(), fs_type.data()));
    return {};
}

} // namespace core
