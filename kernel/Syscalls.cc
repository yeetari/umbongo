#include <kernel/Syscalls.hh>

#include <ustd/Log.hh>
#include <ustd/Types.hh>

uint64 sys_print(const char *str) {
    logln("{}", str);
    return 0;
}
