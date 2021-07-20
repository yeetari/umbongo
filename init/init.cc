#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

void put_char(char ch) {
    Syscall::invoke(Syscall::putchar, ch);
}

int main() {
    core::File file("/home/hello");
    if (!file) {
        return 1;
    }

    Array<char, 20> data{'\0'};
    file.read(data.span());

    uint64 pid = Syscall::invoke(Syscall::getpid);
    logln("[#{}]: {}", pid, static_cast<const char *>(data.data()));
    return static_cast<int>(pid);
}

extern "C" void _start() {
    Syscall::invoke(Syscall::exit, main());
}
