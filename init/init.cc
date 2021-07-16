#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

void put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}

int main() {
    uint64 pid = Syscall::invoke(Syscall::getpid);
    uint64 fd = Syscall::invoke(Syscall::open, "/home/hello");
    Array<char, 20> data{'\0'};
    Syscall::invoke(Syscall::read, fd, data.data(), data.size());
    logln("[#{}]: {}", pid, static_cast<const char *>(data.data()));
    return static_cast<int>(pid);
}

extern "C" void _start() {
    Syscall::invoke(Syscall::exit, main());
}
