#include <core/FileSystem.hh>

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>

namespace core {

ustd::Result<void, SysError> mount(ustd::StringView target, ustd::StringView fs_type) {
    if (auto result = syscall(Syscall::mkdir, target.data());
        result.is_error() && result.error() != SysError::AlreadyExists) {
        return result.error();
    }
    TRY(syscall(Syscall::mount, target.data(), fs_type.data()));
    return {};
}

} // namespace core
