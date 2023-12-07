#include <core/File.hh>
#include <system/Syscall.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<File, ub_error_t> File::open(ustd::StringView path, ub_open_mode_t mode) {
    auto fd = TRY(system::syscall<uint32_t>(UB_SYS_open, path.data(), mode));
    return File(static_cast<uint32_t>(fd));
}

File::~File() {
    close();
}

void File::close() {
    if (m_fd) {
        static_cast<void>(system::syscall(UB_SYS_close, *m_fd));
        m_fd.clear();
    }
}

ustd::Result<size_t, ub_error_t> File::ioctl(ub_ioctl_request_t request, void *arg) {
    return TRY(system::syscall(UB_SYS_ioctl, *m_fd, request, arg));
}

ustd::Result<uintptr_t, ub_error_t> File::mmap() {
    return TRY(system::syscall(UB_SYS_mmap, *m_fd));
}

ustd::Result<size_t, ub_error_t> File::read(ustd::Span<void> data) {
    return TRY(system::syscall(UB_SYS_read, *m_fd, data.data(), data.size()));
}

ustd::Result<size_t, ub_error_t> File::read(ustd::Span<void> data, size_t offset) {
    TRY(system::syscall(UB_SYS_seek, *m_fd, offset, UB_SEEK_MODE_SET));
    return TRY(system::syscall(UB_SYS_read, *m_fd, data.data(), data.size()));
}

ustd::Result<void, ub_error_t> File::rebind(uint32_t fd) {
    TRY(system::syscall(UB_SYS_dup_fd, *m_fd, fd));
    if (*m_fd != fd) {
        TRY(system::syscall(UB_SYS_close, *m_fd));
        m_fd.emplace(fd);
    }
    return {};
}

ustd::Result<size_t, ub_error_t> File::size() {
    return TRY(system::syscall(UB_SYS_size, *m_fd));
}

ustd::Result<size_t, ub_error_t> File::write(ustd::Span<const void> data) {
    return TRY(system::syscall(UB_SYS_write, *m_fd, data.data(), data.size()));
}

} // namespace core
