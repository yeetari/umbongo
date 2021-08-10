#include <assert.h>

#include <ustd/Assert.hh>

extern "C" __attribute__((noreturn)) void __assertion_failed(const char *file, unsigned int line, const char *expr) {
    assertion_failed(file, line, expr);
}
