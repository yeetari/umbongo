#include <core/Directory.hh>

#include <kernel/Syscall.hh>
#include <ustd/Function.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ssize iterate_directory(ustd::StringView path, ustd::Function<void(ustd::StringView)> callback) {
    ssize rc = Syscall::invoke(Syscall::read_directory, path.data(), nullptr);
    if (rc < 0) {
        return rc;
    }
    // TODO: ustd should have some kind of FixedArray container.
    auto byte_count = static_cast<usize>(rc);
    ustd::LargeVector<char> data;
    data.ensure_capacity(byte_count);
    if (rc = Syscall::invoke(Syscall::read_directory, path.data(), data.data()); rc < 0) {
        return rc;
    }
    ssize entry_count = 0;
    for (usize byte_offset = 0; byte_offset != byte_count;) {
        ustd::StringView name(data.data() + byte_offset);
        callback(name);
        byte_offset += name.length() + 1;
        entry_count++;
    }
    return entry_count;
}

} // namespace core
