#include <core/FileSystem.hh>

#include <system/Error.h>
#include <system/Syscall.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>

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
