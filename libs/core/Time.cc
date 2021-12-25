#include <core/Time.hh>

#include <core/Syscall.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>

namespace core {

void sleep(usize ns) {
    EXPECT(syscall(Syscall::poll, nullptr, 0, ns));
}

usize time() {
    return EXPECT(syscall(Syscall::gettime));
}

} // namespace core
