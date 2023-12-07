#include <core/Error.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<File, SysError> File::open(ustd::StringView path, ub_open_mode_t mode) {
    auto fd = TRY(syscall<uint32_t>(Syscall::open, path.data(), mode));
    return File(static_cast<uint32_t>(fd));
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

ustd::Result<size_t, SysError> File::ioctl(ub_ioctl_request_t request, void *arg) {
    return TRY(syscall(Syscall::ioctl, *m_fd, request, arg));
}

ustd::Result<uintptr_t, SysError> File::mmap() {
    return TRY(syscall(Syscall::mmap, *m_fd));
}

ustd::Result<size_t, SysError> File::read(ustd::Span<void> data) {
    return TRY(syscall(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<size_t, SysError> File::read(ustd::Span<void> data, size_t offset) {
    TRY(syscall(Syscall::seek, *m_fd, offset, UB_SEEK_MODE_SET));
    return TRY(syscall(Syscall::read, *m_fd, data.data(), data.size()));
}

ustd::Result<void, SysError> File::rebind(uint32_t fd) {
    TRY(syscall(Syscall::dup_fd, *m_fd, fd));
    if (*m_fd != fd) {
        TRY(syscall(Syscall::close, *m_fd));
        m_fd.emplace(fd);
    }
    return {};
}

ustd::Result<size_t, SysError> File::size() {
    return TRY(syscall(Syscall::size, *m_fd));
}

ustd::Result<size_t, SysError> File::write(ustd::Span<const void> data) {
    return TRY(syscall(Syscall::write, *m_fd, data.data(), data.size()));
}

} // namespace core
