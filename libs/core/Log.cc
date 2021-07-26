#include <kernel/Syscall.hh>

// TODO: So many problems with this.
void put_char(char ch) {
    while (Syscall::invoke(Syscall::write, 1, &ch, 1) != 1) {
    }
}
