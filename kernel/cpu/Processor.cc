#include <kernel/cpu/Processor.hh>

#include <kernel/cpu/PrivilegeLevel.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_gdt_entry_count = 7;
constexpr usize k_interrupt_count = 256;

enum class DescriptorType : uint8 {
    Code = 0b11010,
    Data = 0b10010,
    Tss = 0b01001,
    InterruptGate = 0b01110,
    TrapGate = 0b01111,
};

class [[gnu::packed]] GlobalDescriptor {
    uint16 m_limit_low{0};
    uint16 m_base_low{0};
    uint8 m_base_mid{0};
    DescriptorType m_type : 5 {0};
    PrivilegeLevel m_dpl : 2 {0};
    bool m_present : 1 {false};
    usize m_limit_mid : 4 {0};
    usize m_avl : 1 {0};
    bool m_long_mode : 1 {false};
    bool m_operand_size : 1 {false};
    bool m_granularity : 1 {false};
    uint8 m_base_high{0};

public:
    static constexpr GlobalDescriptor create_kernel(DescriptorType type) { return {PrivilegeLevel::Kernel, type}; }

    constexpr GlobalDescriptor() = default;
    constexpr GlobalDescriptor(PrivilegeLevel dpl, DescriptorType type)
        : m_type(type), m_dpl(dpl), m_present(true), m_long_mode(type == DescriptorType::Code) {}
};

class [[gnu::packed]] InterruptDescriptor {
    uint16 m_base_low{0};
    uint16 m_selector{0};
    uint8 m_ist{0};
    DescriptorType m_type : 5 {0};
    PrivilegeLevel m_dpl : 2 {0};
    bool m_present : 1 {false};
    uint16 m_base_mid{0};
    uint32 m_base_high{0};
    uint32 m_reserved{0};

public:
    static constexpr InterruptDescriptor create_kernel(void (*handler)()) {
        return {PrivilegeLevel::Kernel, DescriptorType::InterruptGate, reinterpret_cast<uintptr>(handler)};
    }

    constexpr InterruptDescriptor() = default;
    constexpr InterruptDescriptor(PrivilegeLevel dpl, DescriptorType type, uint64 base)
        : m_base_low(base & 0xfffful), m_selector(0x08), m_type(type), m_dpl(dpl), m_present(true),
          m_base_mid((base >> 16ul) & 0xfffful), m_base_high((base >> 32ul) & 0xfffffffful) {}
};

template <typename Descriptor, usize Count>
class [[gnu::packed]] DescriptorTable {
    const uint16 m_limit{sizeof(m_descriptors) - 1};
    const uintptr m_base{reinterpret_cast<uintptr>(&m_descriptors)};
    uint64 : 48;
    Array<Descriptor, Count> m_descriptors{};

public:
    DescriptorTable() {
        // AMD64 System Programming Manual revision 3.36 section 4.6.2 recommends that a descriptor table be aligned on
        // a quadword boundary.
        ENSURE(m_base % 8 == 0, "Descriptor table misaligned!");
    }

    void set(uint64 index, const Descriptor &descriptor) {
        ASSERT(index < Count);
        m_descriptors[index] = descriptor;
    }
};

using Gdt = DescriptorTable<GlobalDescriptor, k_gdt_entry_count>;
using Idt = DescriptorTable<InterruptDescriptor, k_interrupt_count>;

Array<InterruptHandler, k_interrupt_count> s_handlers;
Gdt *s_gdt;
Idt *s_idt;

[[noreturn]] void unhandled_interrupt(InterruptFrame *frame) {
    log(" cpu: Received unexpected interrupt {}!", frame->num);
    ENSURE_NOT_REACHED("Unhandled interrupt!");
}

} // namespace

extern uint8 k_interrupt_stubs_start;
extern uint8 k_interrupt_stubs_end;

extern "C" void flush_gdt(Gdt *gdt);
extern "C" void flush_idt(Idt *idt);

extern "C" void interrupt_handler(InterruptFrame *frame) {
    ASSERT_PEDANTIC(frame->num < k_interrupt_count);
    ASSERT_PEDANTIC(s_handlers[frame->num] != nullptr);
    s_handlers[frame->num](frame);
}

void Processor::initialise(void *gdt, void *idt) {
    s_gdt = new (gdt) Gdt;
    s_idt = new (idt) Idt;
    s_gdt->set(1, GlobalDescriptor::create_kernel(DescriptorType::Code));
    s_gdt->set(2, GlobalDescriptor::create_kernel(DescriptorType::Data));
    ENSURE(&k_interrupt_stubs_start != &k_interrupt_stubs_end);
    for (auto *ptr = &k_interrupt_stubs_start; ptr < &k_interrupt_stubs_end; ptr += 12) {
        const uint64 vector = static_cast<uint64>(ptr - &k_interrupt_stubs_start) / 12u;
        ASSERT(vector < k_interrupt_count);
        s_idt->set(vector, InterruptDescriptor::create_kernel(reinterpret_cast<void (*)()>(ptr)));
    }
    flush_gdt(s_gdt);
    flush_idt(s_idt);
    ustd::fill(s_handlers, &unhandled_interrupt);
}

void Processor::wire_interrupt(uint64 vector, InterruptHandler handler) {
    ASSERT(vector < k_interrupt_count);
    ASSERT(handler != nullptr);
    s_handlers[vector] = handler;
}
