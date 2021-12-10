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
    auto fd = Syscall::invoke(Syscall::open, path.data(), OpenMode::None);
    if (fd < 0) {
        return static_cast<SysError>(fd);
    }
    return File(static_cast<uint32>(fd));
}

File::~File() {
    close();
}

void File::close() {
    if (m_fd) {
        Syscall::invoke(Syscall::close, *m_fd);
        m_fd.clear();
    }
}

ssize File::read(ustd::Span<void> data) {
    return Syscall::invoke(Syscall::read, *m_fd, data.data(), data.size());
}

ssize File::read(ustd::Span<void> data, usize offset) {
    if (auto rc = Syscall::invoke(Syscall::seek, *m_fd, offset, SeekMode::Set); rc < 0) {
        return rc;
    }
    return Syscall::invoke(Syscall::read, *m_fd, data.data(), data.size());
}

ssize File::rebind(uint32 fd) {
    if (auto rc = Syscall::invoke(Syscall::dup_fd, *m_fd, fd); rc < 0) {
        return rc;
    }
    if (*m_fd != fd) {
        Syscall::invoke(Syscall::close, *m_fd);
        m_fd.emplace(fd);
    }
    return 0;
}

} // namespace core
