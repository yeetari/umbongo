#include <core/directory.hh>

#include <system/syscall.hh>
#include <ustd/function.hh>
#include <ustd/result.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

namespace core {

ustd::Result<size_t, ub_error_t> iterate_directory(ustd::StringView path,
                                                   ustd::Function<void(ustd::StringView)> callback) {
    // TODO: ustd should have some kind of FixedArray container.
    size_t byte_count = TRY(system::syscall(UB_SYS_read_directory, path.data(), nullptr));
    ustd::LargeVector<char> data;
    data.ensure_capacity(byte_count);
    TRY(system::syscall(UB_SYS_read_directory, path.data(), data.data()));
    size_t entry_count = 0;
    for (size_t byte_offset = 0; byte_offset != byte_count;) {
        ustd::StringView name(data.data() + byte_offset);
        callback(name);
        byte_offset += name.length() + 1;
        entry_count++;
    }
    return entry_count;
}

} // namespace core
