#include <core/FileSystem.hh>

#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ssize mount(ustd::StringView target, ustd::StringView fs_type) {
    if (auto rc = Syscall::invoke(Syscall::mkdir, target.data()); rc < 0 && rc != SysError::AlreadyExists) {
        return rc;
    }
    return Syscall::invoke(Syscall::mount, target.data(), fs_type.data());
}

} // namespace core
