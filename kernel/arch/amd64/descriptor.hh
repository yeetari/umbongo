#pragma once

#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel::arch {

enum class DescriptorType : uint8_t {
    Code = 0b11010,
    Data = 0b10010,
    Tss = 0b01001,
    InterruptGate = 0b01110,
    TrapGate = 0b01111,
};

enum class PrivilegeLevel : uint8_t {
    Kernel = 0,
    User = 3,
};

class [[gnu::packed]] GlobalDescriptor {
    uint16_t m_limit_low{0};
    uint16_t m_base_low{0};
    uint8_t m_base_mid{0};
    DescriptorType m_type : 5 {0};
    PrivilegeLevel m_dpl : 2 {0};
    bool m_present : 1 {false};
    size_t m_limit_mid : 4 {0};
    size_t m_avl : 1 {0};
    bool m_long_mode : 1 {false};
    bool m_operand_size : 1 {false};
    bool m_granularity : 1 {false};
    uint8_t m_base_high{0};

public:
    static constexpr GlobalDescriptor create_base_extended(uint32_t base_extended) { return {base_extended}; }
    static constexpr GlobalDescriptor create_kernel(DescriptorType type) { return {PrivilegeLevel::Kernel, type}; }
    static constexpr GlobalDescriptor create_tss(uint64_t base, uint32_t limit) {
        return {PrivilegeLevel::Kernel, DescriptorType::Tss, base, limit};
    }
    static constexpr GlobalDescriptor create_user(DescriptorType type) { return {PrivilegeLevel::User, type}; }

    constexpr GlobalDescriptor() = default;
    constexpr GlobalDescriptor(uint32_t base_extended)
        : m_limit_low(base_extended & 0xffffu), m_base_low((base_extended >> 16u) & 0xffffu) {}
    constexpr GlobalDescriptor(PrivilegeLevel dpl, DescriptorType type, uint64_t base = 0, uint32_t limit = 0)
        : m_limit_low(limit & 0xffffu), m_base_low(base & 0xffffu), m_base_mid((base >> 16u) & 0xffu), m_type(type),
          m_dpl(dpl), m_present(true), m_limit_mid((limit >> 16u) & 0xffu), m_long_mode(type == DescriptorType::Code),
          m_base_high((base >> 24u) & 0xffu) {}
};

class [[gnu::packed]] InterruptDescriptor {
    uint16_t m_base_low{0};
    uint16_t m_selector{0};
    uint8_t m_ist{0};
    DescriptorType m_type : 5 {0};
    PrivilegeLevel m_dpl : 2 {0};
    bool m_present : 1 {false};
    uint16_t m_base_mid{0};
    uint32_t m_base_high{0};
    uint32_t m_reserved{0};

public:
    static constexpr InterruptDescriptor create_kernel(void (*handler)(), uint8_t ist) {
        return {PrivilegeLevel::Kernel, DescriptorType::InterruptGate, reinterpret_cast<uintptr_t>(handler), ist};
    }

    constexpr InterruptDescriptor() = default;
    constexpr InterruptDescriptor(PrivilegeLevel dpl, DescriptorType type, uint64_t base, uint8_t ist)
        : m_base_low(base & 0xfffful), m_selector(0x08), m_ist(ist), m_type(type), m_dpl(dpl), m_present(true),
          m_base_mid((base >> 16ul) & 0xfffful), m_base_high((base >> 32ul) & 0xfffffffful) {}
};

template <typename Descriptor, size_t Count>
class [[gnu::packed]] DescriptorTable {
    const uint16_t m_limit{sizeof(m_descriptors) - 1};
    const uintptr_t m_base{reinterpret_cast<uintptr_t>(&m_descriptors)};
    alignas(8) ustd::Array<Descriptor, Count> m_descriptors{};

public:
    DescriptorTable() {
        // AMD64 System Programming Manual revision 3.36 section 4.6.2 recommends that a descriptor table be aligned on
        // a quadword boundary.
        ENSURE(m_base % 8 == 0, "Descriptor table misaligned!");
    }

    void set(uint64_t index, const Descriptor &descriptor) {
        ASSERT(index < Count);
        m_descriptors[index] = descriptor;
    }
};

using Gdt = DescriptorTable<GlobalDescriptor, 7>;
using Idt = DescriptorTable<InterruptDescriptor, 256>;

struct [[gnu::packed]] Tss {
    uint32_t : 32;
    void *rsp0;
    void *rsp1;
    void *rsp2;
    uint64_t : 64;
    void *ist1;
    void *ist2;
    void *ist3;
    void *ist4;
    void *ist5;
    void *ist6;
    void *ist7;
    uint64_t : 64;
    uint16_t : 16;
    uint16_t iopb;
};

} // namespace kernel::arch
