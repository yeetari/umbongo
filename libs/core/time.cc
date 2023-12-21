#include <core/time.hh>

#include <system/syscall.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

namespace core {

void sleep(size_t ns) {
    EXPECT(system::syscall(UB_SYS_poll, nullptr, 0, ns));
}

size_t time() {
    return EXPECT(system::syscall(UB_SYS_gettime));
}

} // namespace core
