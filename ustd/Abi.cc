#include <ustd/Assert.hh>

extern "C" int __cxa_atexit(void (*)(void *), void *, void *) {
    return 0;
}

extern "C" [[noreturn]] void __cxa_pure_virtual() {
    ENSURE_NOT_REACHED("__cxa_pure_virtual");
}
