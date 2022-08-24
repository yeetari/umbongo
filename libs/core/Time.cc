#include <core/Time.hh>

#include <core/Syscall.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace core {

void sleep(size_t ns) {
    EXPECT(syscall(Syscall::poll, nullptr, 0, ns));
}

size_t time() {
    return EXPECT(syscall(Syscall::gettime));
}

} // namespace core
