#include <core/Error.hh>
#include <core/File.hh>
#include <core/Print.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

size_t main(size_t argc, const char **argv) {
    ustd::Vector<core::File> files;
    for (size_t i = 1; i < argc; i++) {
        if (auto file = core::File::open(argv[i]); !file.is_error()) {
            files.push(file.disown_value());
        } else {
            core::println("cat: {}: {}", argv[i], core::error_string(file.error()));
            return 1;
        }
    }
    if (files.empty()) {
        files.emplace(0u);
    }
    // TODO: ustd should have some kind of MoveIterator wrapper.
    for (auto &f : files) {
        auto file = ustd::move(f);
        while (true) {
            // NOLINTNEXTLINE
            ustd::Array<uint8_t, 128_KiB> buf;
            auto bytes_read_or_error = file.read(buf.span());
            if (bytes_read_or_error.is_error()) {
                core::println("cat: failed to read from {}: {}", file.fd(),
                              core::error_string(bytes_read_or_error.error()));
                return 1;
            }
            auto bytes_read = bytes_read_or_error.value();
            if (bytes_read == 0) {
                break;
            }
            for (size_t total_written = 0; total_written < bytes_read;) {
                auto bytes_written_or_error =
                    core::syscall(Syscall::write, 1, buf.data() + total_written, bytes_read - total_written);
                if (bytes_written_or_error.is_error()) {
                    core::println("cat: failed to write: {}", core::error_string(bytes_written_or_error.error()));
                    return 1;
                }
                total_written += bytes_written_or_error.value();
            }
        }
    }
    return 0;
}
