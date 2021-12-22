#include <core/Error.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<File, SysError> File::open(ustd::StringView path, OpenMode mode) {
    auto fd = TRY(syscall<uint32>(Syscall::open, path.data(), mode));
    return File(static_cast<uint32>(fd));
}

File::~File() {
    close();
}

void File::close() {
    if (m_fd) {
        static_cast<void>(syscall(Syscall::close, *m_fd));
        m_fd.clear();
    }
}

ustd::Result<usize, SysError> File::ioctl(kernel::IoctlRequest request, void *arg) {
    return TRY(syscall(Syscall::ioctl, *m_fd, request, arg));
}

ustd::Result<uintptr, SysError> File::mmap() {
    return TRY(syscall(Syscall::mmap, *m_fd));
}

ustd::Result<usize, SysError> File::read(ustd::Span<void> data) {
    return TRY(syscall(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<usize, SysError> File::read(ustd::Span<void> data, usize offset) {
    TRY(syscall(Syscall::seek, *m_fd, offset, kernel::SeekMode::Set));
    return TRY(syscall(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<void, SysError> File::rebind(uint32 fd) {
    TRY(syscall(Syscall::dup_fd, *m_fd, fd));
    if (*m_fd != fd) {
        TRY(syscall(Syscall::close, *m_fd));
        m_fd.emplace(fd);
    }
    return {};
}

ustd::Result<usize, SysError> File::write(ustd::Span<const void> data) {
    return TRY(syscall(Syscall::write, *m_fd, data.data(), data.size()));
}

} // namespace core
