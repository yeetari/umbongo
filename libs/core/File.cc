#include <core/File.hh>
#include <kernel/Syscall.hh>

namespace core {

File::File(StringView path) {
    m_fd.emplace(Syscall::invoke(Syscall::open, path.data()));
}

File::~File() {
    if (m_fd) {
        Syscall::invoke(Syscall::close, *m_fd);
    }
}

usize File::read(Span<void> data) {
    return Syscall::invoke(Syscall::read, *m_fd, data.data(), data.size());
}

} // namespace core
