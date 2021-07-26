#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

int main() {
    while (true) {
        // NOLINTNEXTLINE
        Array<char, 8192> buf;
        // TODO: This is non-blocking.
        auto nread = Syscall::invoke(Syscall::read, 0, buf.data(), buf.size());
        if (nread < 0) {
            ENSURE_NOT_REACHED();
        } else if (nread == 0) {
            continue;
        }

        // TODO: Console server should manage the framebuffer itself.
        for (usize i = 0; i < static_cast<usize>(nread); i++) {
            Syscall::invoke(Syscall::putchar, buf[i]);
        }
    }
}
