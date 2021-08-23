#include <core/Error.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

usize main(usize argc, const char **argv) {
    Vector<uint32> fds;
    for (usize i = 1; i < argc; i++) {
        ssize fd = Syscall::invoke(Syscall::open, argv[i], OpenMode::None);
        if (fd < 0) {
            printf("cat: {}: {}\n", argv[i], core::error_string(fd));
            return 1;
        }
        fds.push(static_cast<uint32>(fd));
    }
    if (fds.empty()) {
        fds.push(0);
    }

    for (uint32 fd : fds) {
        while (true) {
            // NOLINTNEXTLINE
            Array<uint8, 128_KiB> buf;
            auto bytes_read = Syscall::invoke(Syscall::read, fd, buf.data(), buf.size());
            if (bytes_read == 0) {
                break;
            }
            if (bytes_read < 0) {
                printf("cat: failed to read from {}: {}\n", fd, core::error_string(bytes_read));
                return 1;
            }
            for (usize total_written = 0; total_written < static_cast<usize>(bytes_read);) {
                auto bytes_written = Syscall::invoke(Syscall::write, 1, buf.data() + total_written,
                                                     static_cast<usize>(bytes_read) - total_written);
                if (bytes_written < 0) {
                    printf("cat: failed to write: {}\n", core::error_string(bytes_written));
                    return 1;
                }
                total_written += static_cast<usize>(bytes_written);
            }
        }
        Syscall::invoke(Syscall::close, fd);
    }
    return 0;
}
