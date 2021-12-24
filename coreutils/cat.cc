#include <core/Error.hh>
#include <core/Print.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

usize main(usize argc, const char **argv) {
    // TODO: Use Core::file
    ustd::Vector<uint32> fds;
    for (usize i = 1; i < argc; i++) {
        auto fd = core::syscall<uint32>(Syscall::open, argv[i], kernel::OpenMode::None);
        if (fd.is_error()) {
            core::println("cat: {}: {}", argv[i], core::error_string(fd.error()));
            return 1;
        }
        fds.push(fd.value());
    }
    if (fds.empty()) {
        fds.push(0);
    }

    for (uint32 fd : fds) {
        while (true) {
            // NOLINTNEXTLINE
            ustd::Array<uint8, 128_KiB> buf;
            auto bytes_read_or_error = core::syscall(Syscall::read, fd, buf.data(), buf.size());
            if (bytes_read_or_error.is_error()) {
                core::println("cat: failed to read from {}: {}", fd, core::error_string(bytes_read_or_error.error()));
                return 1;
            }
            auto bytes_read = bytes_read_or_error.value();
            if (bytes_read == 0) {
                break;
            }
            for (usize total_written = 0; total_written < bytes_read;) {
                auto bytes_written_or_error =
                    core::syscall(Syscall::write, 1, buf.data() + total_written, bytes_read - total_written);
                if (bytes_written_or_error.is_error()) {
                    core::println("cat: failed to write: {}", core::error_string(bytes_written_or_error.error()));
                    return 1;
                }
                total_written += bytes_written_or_error.value();
            }
        }
        EXPECT(core::syscall(Syscall::close, fd));
    }
    return 0;
}
