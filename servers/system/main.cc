#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

int main() {
    if (Syscall::invoke(Syscall::create_process, "/console-server") < 0) {
        ENSURE_NOT_REACHED("Failed to start console server!");
    }

    core::File file("/home/hello");
    ENSURE(file, "Failed to open /home/hello");

    Array<char, 20> data{'\0'};
    file.read(data.span());

    auto pid = Syscall::invoke<uint64>(Syscall::getpid);
    logln("[#{}]: {}", pid, static_cast<const char *>(data.data()));
    return static_cast<int>(pid);
}
