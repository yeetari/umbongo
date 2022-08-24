#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/Types.hh>

namespace {

uintptr_t s_pos = 6_TiB;

} // namespace

void *operator new(size_t size) {
    s_pos = ustd::align_up(s_pos, 16);
    auto *ret = reinterpret_cast<void *>(s_pos);
    s_pos += size;
    return ret;
}

void *operator new[](size_t size) {
    return operator new(size);
}

void *operator new(size_t size, ustd::align_val_t align) {
    s_pos = ustd::align_up(s_pos, static_cast<size_t>(align));
    auto *ret = reinterpret_cast<void *>(s_pos);
    s_pos += size;
    return ret;
}

void *operator new[](size_t size, ustd::align_val_t align) {
    return operator new(size, align);
}

void operator delete(void *) {}
void operator delete[](void *) {}
