#pragma once

#include <ustd/Types.hh>

template <typename T = uint8>
T port_read(uint16 port) requires(sizeof(T) == 1) {
    T value;
    asm volatile("in %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

template <typename T>
void port_write(uint16 port, T value) requires(sizeof(T) == 1) {
    asm volatile("out %0, %1" : : "a"(value), "Nd"(port));
}
