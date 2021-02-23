#include <kernel/cpu/Processor.hh>

#include <kernel/cpu/PrivilegeLevel.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
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
    static constexpr GlobalDescriptor create_base_extended(uint32 base_extended) { return {base_extended}; }
    static constexpr GlobalDescriptor create_kernel(DescriptorType type) { return {PrivilegeLevel::Kernel, type}; }
    static constexpr GlobalDescriptor create_tss(uint64 base, uint32 limit) {
        return {PrivilegeLevel::Kernel, DescriptorType::Tss, base, limit};
    }
    static constexpr GlobalDescriptor create_user(DescriptorType type) { return {PrivilegeLevel::User, type}; }

    constexpr GlobalDescriptor() = default;
    constexpr GlobalDescriptor(uint32 base_extended) // NOLINT
        : m_limit_low(base_extended & 0xffffu), m_base_low((base_extended >> 16u) & 0xffffu) {}
    constexpr GlobalDescriptor(PrivilegeLevel dpl, DescriptorType type, uint64 base = 0, uint32 limit = 0)
        : m_limit_low(limit & 0xffffu), m_base_low(base & 0xffffu), m_base_mid((base >> 16u) & 0xffu), m_type(type),
          m_dpl(dpl), m_present(true), m_limit_mid((limit >> 16u) & 0xffu), m_long_mode(type == DescriptorType::Code),
          m_base_high((base >> 24u) & 0xffu) {}
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

using Gdt = DescriptorTable<GlobalDescriptor, k_gdt_entry_count>;
using Idt = DescriptorTable<InterruptDescriptor, k_interrupt_count>;

struct [[gnu::packed]] Tss {
    uint32 : 32;
    void *rsp0;
    void *rsp1;
    void *rsp2;
    uint64 : 64;
    void *ist1;
    void *ist2;
    void *ist3;
    void *ist4;
    void *ist5;
    void *ist6;
    void *ist7;
    uint64 : 64;
    uint16 : 16;
    uint16 iopb;
};

Array<InterruptHandler, k_interrupt_count> s_interrupt_table;

[[noreturn]] void unhandled_interrupt(InterruptFrame *frame) {
    logln(" cpu: Received unexpected interrupt {} in ring {}!", frame->num, frame->cs & 3u);
    ENSURE_NOT_REACHED("Unhandled interrupt!");
}

} // namespace

extern uint8 k_interrupt_stubs_start;
extern uint8 k_interrupt_stubs_end;

extern "C" void flush_gdt(Gdt *gdt);

extern "C" void interrupt_handler(InterruptFrame *frame) {
    ASSERT_PEDANTIC(frame->num < k_interrupt_count);
    ASSERT_PEDANTIC(s_interrupt_table[frame->num] != nullptr);
    s_interrupt_table[frame->num](frame);
}

void Processor::initialise() {
    auto *gdt = new Gdt;
    auto *idt = new Idt;
    auto *tss = new Tss;

    // Setup flat GDT.
    gdt->set(1, GlobalDescriptor::create_kernel(DescriptorType::Code));
    gdt->set(2, GlobalDescriptor::create_kernel(DescriptorType::Data));
    gdt->set(3, GlobalDescriptor::create_user(DescriptorType::Data));
    gdt->set(4, GlobalDescriptor::create_user(DescriptorType::Code));
    gdt->set(5, GlobalDescriptor::create_tss(reinterpret_cast<uintptr>(tss), sizeof(Tss) - 1));
    gdt->set(6, GlobalDescriptor::create_base_extended(reinterpret_cast<uintptr>(tss) >> 32u));

    // Fill IDT with asm handler stubs. Also make sure that the interrupt stub section isn't empty, which could happen
    // if the linker accidentally removes it.
    ENSURE(&k_interrupt_stubs_start != &k_interrupt_stubs_end);
    for (auto *ptr = &k_interrupt_stubs_start; ptr < &k_interrupt_stubs_end; ptr += 12) {
        const uint64 vector = static_cast<uint64>(ptr - &k_interrupt_stubs_start) / 12u;
        idt->set(vector, InterruptDescriptor::create_kernel(reinterpret_cast<void (*)()>(ptr)));
    }

    // Set kernel stack for interrupt handling. Also set IO permission bitmap base to `sizeof(Tss)`. This makes any ring
    // 3 attempt to use IO ports fail with a #GP exception since the TSS limit is also its size.
    tss->rsp0 = new char[4_KiB] + 4_KiB;
    tss->iopb = sizeof(Tss);

    // Load our new GDT, IDT and TSS. Flushing the GDT involves performing an iret to change the current code selector.
    // We also fill the interrupt handler table with trap handlers to ensure any unexpected interrupts are caught.
    flush_gdt(gdt);
    asm volatile("lidt %0" : : "m"(*idt));
    asm volatile("ltr %%ax" : : "a"(0x28));
    ustd::fill(s_interrupt_table, &unhandled_interrupt);
}

void Processor::wire_interrupt(uint64 vector, InterruptHandler handler) {
    ASSERT(vector < k_interrupt_count);
    ASSERT(handler != nullptr);
    s_interrupt_table[vector] = handler;
}

void Processor::write_cr3(void *pml4) {
    // Make sure both the upper and lower 12 bits of the PML4 base address are clear. The upper 12 bits of CR3 should be
    // clear since the minimum physical address width required to be supported by the CPU is 40 bits. The lower 12 bits
    // of CR3 are used for flags and should be clear in the PML4 base address since it should be page-aligned.
    ASSERT_PEDANTIC((reinterpret_cast<uintptr>(pml4) & 0xfff0000000000fffu) == 0);
    asm volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}
