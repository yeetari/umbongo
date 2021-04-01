#include <kernel/cpu/Processor.hh>

#include <kernel/Syscall.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/PrivilegeLevel.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_gdt_entry_count = 7;
constexpr usize k_interrupt_count = 256;

constexpr uint32 k_msr_efer = 0xc0000080;
constexpr uint32 k_msr_star = 0xc0000081;
constexpr uint32 k_msr_lstar = 0xc0000082;
constexpr uint32 k_msr_sfmask = 0xc0000084;
constexpr uint32 k_msr_kernel_gs_base = 0xc0000102;

class CpuId {
    uint32 m_eax{0};
    uint32 m_ebx{0};
    uint32 m_ecx{0};
    uint32 m_edx{0};

public:
    explicit CpuId(uint32 function, uint32 sub_function = 0) {
        asm volatile("cpuid" : "=a"(m_eax), "=b"(m_ebx), "=c"(m_ecx), "=d"(m_edx) : "a"(function), "c"(sub_function));
    }

    uint32 eax() const { return m_eax; }
    uint32 ebx() const { return m_ebx; }
    uint32 ecx() const { return m_ecx; }
    uint32 edx() const { return m_edx; }
};

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

// A struct to store per core information. Packed because we access some fields from assembly.
struct [[gnu::packed]] LocalStorage {
    LocalStorage *self{this};
    void *kernel_stack{nullptr};
    void *user_stack{nullptr};
    Process *current_process{nullptr};
};

struct [[gnu::packed]] SyscallFrame {
    uint64 r15, r14, r13, r12, r11, r10, r9, r8, rbp, rsi, rdi, rdx, rcx, rbx, rax;
};

// For some reason the clang global constructor warning thinks the syscall table requires global constructors, even
// though it doesn't.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wglobal-constructors"
Array<InterruptHandler, k_interrupt_count> s_interrupt_table;
using SyscallHandler = uint64 (Process::*)(uint64, uint64, uint64);
#define ENUMERATE_SYSCALL(s) reinterpret_cast<SyscallHandler>(&Process::sys_##s),
Array<SyscallHandler, Syscall::__Count__> s_syscall_table{ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)};
#undef ENUMERATE_SYSCALL
#pragma clang diagnostic pop

LocalApic *s_apic = nullptr;

uint64 read_cr0() {
    uint64 cr0 = 0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

uint64 read_cr4() {
    uint64 cr4 = 0;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

void write_cr0(uint64 cr0) {
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void write_cr4(uint64 cr4) {
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}

uint64 read_msr(uint32 msr) {
    uint64 rax = 0;
    uint64 rdx = 0;
    asm volatile("rdmsr" : "=a"(rax), "=d"(rdx) : "c"(msr));
    return rax | (rdx << 32u);
}

void write_msr(uint32 msr, uint64 value) {
    const uint64 rax = value & 0xffffffffu;
    const uint64 rdx = (value >> 32u) & 0xffffffffu;
    asm volatile("wrmsr" : : "a"(rax), "d"(rdx), "c"(msr));
}

[[noreturn]] void unhandled_interrupt(RegisterState *regs) {
    logln(" cpu: Received unexpected interrupt {} in ring {}!", regs->int_num, regs->cs & 3u);
    ENSURE_NOT_REACHED("Unhandled interrupt!");
}

} // namespace

extern uint8 k_interrupt_stubs_start;
extern uint8 k_interrupt_stubs_end;

extern "C" void flush_gdt(Gdt *gdt);
extern "C" void switch_now(RegisterState *regs);
extern "C" void syscall_stub();

extern "C" void interrupt_handler(RegisterState *regs) {
    ASSERT_PEDANTIC(regs->int_num < k_interrupt_count);
    ASSERT_PEDANTIC(s_interrupt_table[regs->int_num] != nullptr);
    const bool should_send_eoi = regs->int_num >= 32;
    s_interrupt_table[regs->int_num](regs);
    if (should_send_eoi) {
        ASSERT_PEDANTIC(s_apic != nullptr);
        s_apic->send_eoi();
    }
}

extern "C" void syscall_handler(SyscallFrame *frame, LocalStorage *storage) {
    // Syscall number passed in rax.
    if (frame->rax >= s_syscall_table.size()) {
        return;
    }
    auto *process = storage->current_process;
    ASSERT_PEDANTIC(process != nullptr);
    ASSERT_PEDANTIC(s_syscall_table[frame->rax] != nullptr);
    const bool is_exit = frame->rax == Syscall::exit;
    frame->rax = (process->*s_syscall_table[frame->rax])(frame->rdi, frame->rsi, frame->rdx);
    if (is_exit) {
        process->kill();
        RegisterState regs{};
        Scheduler::switch_next(&regs);
        switch_now(&regs);
    }
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

    // Allocate CPU local storage struct and allocate some kernel stack for syscalls/interrupt handling.
    auto *storage = new LocalStorage;
    storage->kernel_stack = new char[4_KiB] + 4_KiB;
    write_msr(k_msr_kernel_gs_base, reinterpret_cast<uintptr>(storage));

    // Set kernel stack for interrupt handling. Also set IO permission bitmap base to `sizeof(Tss)`. This makes any ring
    // 3 attempt to use IO ports fail with a #GP exception since the TSS limit is also its size.
    tss->rsp0 = storage->kernel_stack;
    tss->iopb = sizeof(Tss);

    // Load our new GDT, IDT and TSS. Flushing the GDT involves performing an iret to change the current code selector.
    // We also fill the interrupt handler table with trap handlers to ensure any unexpected interrupts are caught.
    flush_gdt(gdt);
    asm volatile("lidt %0" : : "m"(*idt));
    asm volatile("ltr %%ax" : : "a"(0x28));
    ustd::fill(s_interrupt_table, &unhandled_interrupt);

    // Enable CR0.WP (Write Protect). This prevents the kernel from writing to read only pages.
    write_cr0(read_cr0() | (1u << 16u));

    // Enable some hardware protection features, if available.
    CpuId extended_features(0x7);
    if ((extended_features.ebx() & (1u << 7u)) != 0) {
        // Enable CR4.SMEP (Supervisor Memory Execute Protection). This prevents the kernel from executing user
        // accessible code.
        logln(" cpu: Enabling SMEP");
        write_cr4(read_cr4() | (1u << 20u));
    }
    if ((extended_features.ecx() & (1u << 2u)) != 0) {
        // Enable CR4.UMIP (User-Mode Instruction Prevention). This prevents user code from executing instructions like
        // sgdt and sidt.
        logln(" cpu: Enabling UMIP");
        write_cr4(read_cr4() | (1u << 11u));
    }

    // Perform an extended cpuid and ensure that the syscall/sysret, NX and 1GiB page features are available.
    CpuId extended_cpu_id(0x80000001);
    ENSURE((extended_cpu_id.edx() & (1u << 11u)) != 0, "syscall/sysret not available!");
    ENSURE((extended_cpu_id.edx() & (1u << 20u)) != 0, "NX not available!");
    ENSURE((extended_cpu_id.edx() & (1u << 26u)) != 0, "1GiB pages not available!");

    // Enable EFER.SCE (System Call Extensions) and EFER.NXE (No-Execute Enable).
    write_msr(k_msr_efer, read_msr(k_msr_efer) | (1u << 0u) | (1u << 11u));

    // Write selectors to STAR MSR. First write the sysret CS and SS (63:48), then write the syscall CS and SS (47:32).
    // The bottom 32 bits for the target EIP are not used in long mode. 0x13 is used for the sysret CS/SS pair because
    // CS is set to 0x13+16 (0x23), which is the user code segment and SS is set to 0x13+8 (0x1b) which is the user data
    // segment. This is also the reason user code comes after user data in the GDT. 0x08 is used for the syscall CS/SS
    // pair because CS is set to 0x08, which is the kernel code segment and SS is set to 0x08+8 (0x10), which is the
    // kernel data segment.
    uint64 star = 0;
    star |= 0x13ul << 48u;
    star |= 0x08ul << 32u;
    write_msr(k_msr_star, star);

    // Write syscall entry point to the LSTAR MSR. Also set the CPU to mask the IF bit when a syscall occurs.
    write_msr(k_msr_lstar, reinterpret_cast<uintptr>(&syscall_stub));
    write_msr(k_msr_sfmask, 1u << 9u);
}

void Processor::set_apic(LocalApic *apic) {
    ASSERT(s_apic == nullptr);
    s_apic = apic;
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

LocalApic *Processor::apic() {
    ASSERT_PEDANTIC(s_apic != nullptr);
    return s_apic;
}
