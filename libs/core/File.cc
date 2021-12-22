#include <core/File.hh>
#include <kernel/SysError.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<File, SysError> File::open(ustd::StringView path) {
    auto fd = TRY(Syscall::invoke<uint32>(Syscall::open, path.data(), OpenMode::None));
    return File(static_cast<uint32>(fd));
}

File::~File() {
    close();
}

void File::close() {
    if (m_fd) {
        static_cast<void>(Syscall::invoke(Syscall::close, *m_fd));
        m_fd.clear();
    }
}

ustd::Result<usize, SysError> File::ioctl(IoctlRequest request, void *arg) {
    return TRY(Syscall::invoke(Syscall::ioctl, *m_fd, request, arg));
}

ustd::Result<uintptr, SysError> File::mmap() {
    return TRY(Syscall::invoke(Syscall::mmap, *m_fd));
}

ustd::Result<usize, SysError> File::read(ustd::Span<void> data) {
    return TRY(Syscall::invoke(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<usize, SysError> File::read(ustd::Span<void> data, usize offset) {
    TRY(Syscall::invoke(Syscall::seek, *m_fd, offset, SeekMode::Set));
    return TRY(Syscall::invoke(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<void, SysError> File::rebind(uint32 fd) {
    TRY(Syscall::invoke(Syscall::dup_fd, *m_fd, fd));
    if (*m_fd != fd) {
        TRY(Syscall::invoke(Syscall::close, *m_fd));
        m_fd.emplace(fd);
    }
    return {};
}

ustd::Result<usize, SysError> File::write(ustd::Span<const void> data) {
    return TRY(Syscall::invoke(Syscall::write, *m_fd, data.data(), data.size()));
}

} // namespace core
