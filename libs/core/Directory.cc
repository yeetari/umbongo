#include <core/Directory.hh>

#include <core/Error.hh>
#include <core/Syscall.hh>
#include <ustd/Function.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace core {

ustd::Result<usize, SysError> iterate_directory(ustd::StringView path,
                                                ustd::Function<void(ustd::StringView)> callback) {
    // TODO: ustd should have some kind of FixedArray container.
    usize byte_count = TRY(syscall(Syscall::read_directory, path.data(), nullptr));
    ustd::LargeVector<char> data;
    data.ensure_capacity(byte_count);
    TRY(syscall(Syscall::read_directory, path.data(), data.data()));
    usize entry_count = 0;
    for (usize byte_offset = 0; byte_offset != byte_count;) {
        ustd::StringView name(data.data() + byte_offset);
        callback(name);
        byte_offset += name.length() + 1;
        entry_count++;
    }
    return entry_count;
}

} // namespace core
