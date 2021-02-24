#include <kernel/Syscall.hh>

#include <ustd/Log.hh>
#include <ustd/Types.hh>

uint64 sys_print() {
    logln("Received print syscall!");
    return 0;
}
