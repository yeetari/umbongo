#include <core/Time.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

void sleep(size_t ns) {
    EXPECT(system::syscall(UB_SYS_poll, nullptr, 0, ns));
}

size_t time() {
    return EXPECT(system::syscall(UB_SYS_gettime));
}

} // namespace core
