#include <core/Time.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

void sleep(size_t) {
    ENSURE_NOT_REACHED("TODO: Implement");
}

size_t uptime() {
    return ASSUME(system::syscall<size_t>(SYS_uptime));
}

} // namespace core
