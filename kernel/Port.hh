#pragma once

#include <ustd/Types.hh>

namespace kernel {

template <typename T = uint8_t>
T port_read(uint16_t port) requires(sizeof(T) <= 4) {
    T value;
    asm volatile("in %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

template <typename T>
void port_write(uint16_t port, T value) requires(sizeof(T) <= 4) {
    asm volatile("out %0, %1" : : "a"(value), "Nd"(port));
}

} // namespace kernel
