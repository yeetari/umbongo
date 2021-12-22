#include <core/FileSystem.hh>

#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>

namespace core {

ustd::Result<void, SysError> mount(ustd::StringView target, ustd::StringView fs_type) {
    if (auto result = Syscall::invoke(Syscall::mkdir, target.data());
        result.is_error() && result.error() != SysError::AlreadyExists) {
        return result.error();
    }
    TRY(Syscall::invoke(Syscall::mount, target.data(), fs_type.data()));
    return {};
}

} // namespace core
