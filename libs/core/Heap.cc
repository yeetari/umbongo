#include <ustd/Numeric.hh>
#include <ustd/Types.hh>

namespace {

uintptr s_pos = 6_TiB;

} // namespace

void *operator new(usize size) {
    s_pos = ustd::round_up(s_pos, 16);
    auto *ret = reinterpret_cast<void *>(s_pos);
    s_pos += size;
    return ret;
}

void *operator new[](usize size) {
    return operator new(size);
}

void operator delete(void *) noexcept {}
void operator delete[](void *) noexcept {}
