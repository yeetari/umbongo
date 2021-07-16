#include <kernel/proc/Process.hh>

#include <ustd/Log.hh>
#include <ustd/Types.hh>

uint64 Process::sys_exit(uint64 code) const {
    logln("[#{}]: sys_exit called with code {}", m_pid, code);
    return 0;
}

uint64 Process::sys_getpid() const {
    return m_pid;
}

uint64 Process::sys_putchar(char ch) const {
    put_char(ch);
    return 0;
}
