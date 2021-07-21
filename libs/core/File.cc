#include <core/File.hh>
#include <kernel/Syscall.hh>

namespace core {

File::File(StringView path) {
    auto fd = Syscall::invoke(Syscall::open, path.data());
    if (fd >= 0) {
        m_fd.emplace(static_cast<uint32>(fd));
    }
}

File::~File() {
    if (m_fd) {
        Syscall::invoke(Syscall::close, *m_fd);
    }
}

usize File::read(Span<void> data) {
    return Syscall::invoke<usize>(Syscall::read, *m_fd, data.data(), data.size());
}

} // namespace core
