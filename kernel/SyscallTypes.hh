#pragma once

#include <ustd/Types.hh>

enum class MemoryProt : usize {
    Write = 1u << 0u,
    Exec = 1u << 1u,
};

inline constexpr MemoryProt operator&(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<usize>(a) & static_cast<usize>(b));
}

inline constexpr MemoryProt operator|(MemoryProt a, MemoryProt b) {
    return static_cast<MemoryProt>(static_cast<usize>(a) | static_cast<usize>(b));
}
