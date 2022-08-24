#include <kernel/cpu/Processor.hh>

#include <kernel/Dmesg.hh>
#include <kernel/SysResult.hh>
#include <kernel/Syscall.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/InterruptController.hh>
#include <kernel/cpu/LocalApic.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/Memory.hh>
#include <ustd/ScopeGuard.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

extern uint8_t k_interrupt_stubs_start;
extern uint8_t k_interrupt_stubs_end;

namespace kernel {
namespace {

constexpr size_t k_gdt_entry_count = 7;
constexpr size_t k_interrupt_count = 256;

constexpr uint32_t k_msr_efer = 0xc0000080;
constexpr uint32_t k_msr_star = 0xc0000081;
constexpr uint32_t k_msr_lstar = 0xc0000082;
constexpr uint32_t k_msr_sfmask = 0xc0000084;
constexpr uint32_t k_msr_gs_base = 0xc0000101;

class CpuId {
    uint32_t m_eax{0};
    uint32_t m_ebx{0};
    uint32_t m_ecx{0};
    uint32_t m_edx{0};

public:
    explicit CpuId(uint32_t function, uint32_t sub_function = 0) {
        asm volatile("cpuid" : "=a"(m_eax), "=b"(m_ebx), "=c"(m_ecx), "=d"(m_edx) : "a"(function), "c"(sub_function));
    }

    uint32_t eax() const { return m_eax; }
    uint32_t ebx() const { return m_ebx; }
    uint32_t ecx() const { return m_ecx; }
    uint32_t edx() const { return m_edx; }
};

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

template <typename Descriptor, size_t Count>
class [[gnu::packed]] DescriptorTable {
    const uint16_t m_limit{sizeof(m_descriptors) - 1};
    const uintptr_t m_base{reinterpret_cast<uintptr_t>(&m_descriptors)};
    uint64_t : 48;
    ustd::Array<Descriptor, Count> m_descriptors{};

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
    constexpr GlobalDescriptor(uint32_t base_extended) // NOLINT
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

using Gdt = DescriptorTable<GlobalDescriptor, k_gdt_entry_count>;
using Idt = DescriptorTable<InterruptDescriptor, k_interrupt_count>;

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

// A struct to store per core information. Packed because we access some fields from assembly.
struct [[gnu::packed]] LocalStorage {
    LocalStorage *self{this};
    void *kernel_stack{nullptr};
    void *user_stack{nullptr};
    Thread *current_thread{nullptr};
    VirtSpace *current_space{nullptr};
    uint8_t id{0};
};

struct [[gnu::packed]] SyscallFrame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rflags;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rip;
    uint64_t rbx;
    uint64_t rax;
};

ustd::Array<InterruptHandler, k_interrupt_count> s_interrupt_table;
using SyscallHandler = SyscallResult (Process::*)(uint64_t, uint64_t, uint64_t);
#define ENUMERATE_SYSCALL(s) reinterpret_cast<SyscallHandler>(&Process::sys_##s),
const ustd::Array<SyscallHandler, Syscall::__Count__> s_syscall_table{ENUMERATE_SYSCALLS(ENUMERATE_SYSCALL)};
#undef ENUMERATE_SYSCALL

LocalApic *s_apic = nullptr;
ustd::Atomic<uint8_t> s_initialised_ap_count = 0;
uint8_t *s_simd_default_region = nullptr;
uint32_t s_simd_region_size = 0;

uint64_t read_cr0() {
    uint64_t cr0 = 0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    return cr0;
}

uint64_t read_cr4() {
    uint64_t cr4 = 0;
    asm volatile("mov %%cr4, %0" : "=r"(cr4));
    return cr4;
}

void write_cr0(uint64_t cr0) {
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void write_cr4(uint64_t cr4) {
    asm volatile("mov %0, %%cr4" : : "r"(cr4));
}

void write_xcr(uint64_t value) {
    const uint64_t rax = value & 0xffffffffu;
    const uint64_t rdx = (value >> 32u) & 0xffffffffu;
    asm volatile("xsetbv" : : "a"(rax), "d"(rdx), "c"(0));
}

uint64_t read_gs(uint64_t offset) {
    uint64_t value = 0;
    asm volatile("mov %%gs:%a[off], %[val]" : [val] "=r"(value) : [off] "ir"(offset));
    return value;
}

void write_gs(uint64_t offset, uint64_t value) {
    asm volatile("mov %[val], %%gs:%a[off]" : : [off] "ir"(offset), [val] "r"(value));
}

uint64_t read_msr(uint32_t msr) {
    uint64_t rax = 0;
    uint64_t rdx = 0;
    asm volatile("rdmsr" : "=a"(rax), "=d"(rdx) : "c"(msr));
    return rax | (rdx << 32u);
}

void write_msr(uint32_t msr, uint64_t value) {
    const uint64_t rax = value & 0xffffffffu;
    const uint64_t rdx = (value >> 32u) & 0xffffffffu;
    asm volatile("wrmsr" : : "a"(rax), "d"(rdx), "c"(msr));
}

[[noreturn]] void unhandled_interrupt(RegisterState *regs) {
    dmesg(" cpu: Received unexpected interrupt {} in ring {}!", regs->int_num, regs->cs & 3u);
    ENSURE_NOT_REACHED("Unhandled interrupt!");
}

} // namespace

extern "C" void ap_bootstrap();
extern "C" void ap_bootstrap_end();
extern "C" uintptr_t *ap_prepare();
extern "C" void flush_gdt(Gdt *gdt);
extern "C" void syscall_stub();

extern "C" void ap_entry(uint8_t id) {
    s_initialised_ap_count.fetch_add(1, ustd::MemoryOrder::AcqRel);
    Processor::setup(id);
    Scheduler::setup();
    Scheduler::start();
}

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

extern "C" void syscall_handler(SyscallFrame *frame, Thread *thread) {
    // Syscall number passed in rax.
    if (frame->rax >= s_syscall_table.size()) {
        return;
    }
    ASSERT_PEDANTIC(thread != nullptr);
    auto *process = &thread->process();
    ASSERT_PEDANTIC(process != nullptr);
    ASSERT_PEDANTIC(s_syscall_table[frame->rax] != nullptr);
    const auto result = (process->*s_syscall_table[frame->rax])(frame->rdi, frame->rsi, frame->rdx);
    frame->rax = result.value();
}

void Processor::initialise() {
    // Create and fill an IDT with our asm handler stubs. Also make sure that the interrupt stub section isn't empty,
    // which could happen if the linker accidentally removes it.
    ENSURE(&k_interrupt_stubs_start != &k_interrupt_stubs_end);
    auto *idt = new Idt;
    for (auto *ptr = &k_interrupt_stubs_start; ptr < &k_interrupt_stubs_end; ptr += 12) {
        const uint64_t vector = static_cast<uint64_t>(ptr - &k_interrupt_stubs_start) / 12u;
        idt->set(vector, InterruptDescriptor::create_kernel(reinterpret_cast<void (*)()>(ptr), vector >= 32 ? 1 : 0));
    }

    // Load our new IDT and fill the interrupt handler table with trap handlers to ensure any unexpected interrupts are
    // caught.
    asm volatile("lidt %0" : : "m"(*idt));
    ustd::fill(s_interrupt_table, &unhandled_interrupt);

    // Print CPU vendor.
    CpuId vendor_cpu_id(0);
    dmesg(
        " cpu: Vendor '{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}{:c}'", (vendor_cpu_id.ebx() >> 0u) & 0xffu,
        (vendor_cpu_id.ebx() >> 8u) & 0xffu, (vendor_cpu_id.ebx() >> 16u) & 0xffu, (vendor_cpu_id.ebx() >> 24u) & 0xffu,
        (vendor_cpu_id.edx() >> 0u) & 0xffu, (vendor_cpu_id.edx() >> 8u) & 0xffu, (vendor_cpu_id.edx() >> 16u) & 0xffu,
        (vendor_cpu_id.edx() >> 24u) & 0xffu, (vendor_cpu_id.ecx() >> 0u) & 0xffu, (vendor_cpu_id.ecx() >> 8u) & 0xffu,
        (vendor_cpu_id.ecx() >> 16u) & 0xffu, (vendor_cpu_id.ecx() >> 24u) & 0xffu);

    // Enable CR0.WP (Write Protect). This prevents the kernel from writing to read only pages.
    write_cr0(read_cr0() | (1u << 16u));

    // Enable CR4.PGE (Page-Global Enable). This enables support for global pages.
    write_cr4(read_cr4() | (1u << 7u));

    // Enable some hardware protection features, if available.
    CpuId extended_features(0x7);
    if ((extended_features.ebx() & (1u << 7u)) != 0) {
        // Enable CR4.SMEP (Supervisor Memory Execute Protection). This prevents the kernel from executing user
        // accessible code.
        dmesg(" cpu: Enabling SMEP");
        write_cr4(read_cr4() | (1u << 20u));
    }
    if ((extended_features.ecx() & (1u << 2u)) != 0) {
        // Enable CR4.UMIP (User-Mode Instruction Prevention). This prevents user code from executing instructions like
        // sgdt and sidt.
        dmesg(" cpu: Enabling UMIP");
        write_cr4(read_cr4() | (1u << 11u));
    }

    // Perform an extended cpuid and ensure that the syscall/sysret, NX and 1 GiB page features are available.
    CpuId extended_cpu_id(0x80000001);
    ENSURE((extended_cpu_id.edx() & (1u << 11u)) != 0, "syscall/sysret not available!");
    ENSURE((extended_cpu_id.edx() & (1u << 20u)) != 0, "NX not available!");
    ENSURE((extended_cpu_id.edx() & (1u << 26u)) != 0, "1GiB pages not available!");

    // Enable EFER.SCE (System Call Extensions) and EFER.NXE (No-Execute Enable).
    write_msr(k_msr_efer, read_msr(k_msr_efer) | (1u << 11u) | (1u << 0u));

    CpuId sse_cpu_id(1);
    ENSURE((sse_cpu_id.edx() & (1u << 25u)) != 0, "SSE1 not available!");
    ENSURE((sse_cpu_id.edx() & (1u << 26u)) != 0, "SSE2 not available!");
    ENSURE((sse_cpu_id.ecx() & (1u << 26u)) != 0, "xsave/xrstor not available!");

    // Enable SSE instructions.
    write_cr0((read_cr0() & ~(1u << 2u)) | (1u << 1u));
    write_cr4(read_cr4() | (1u << 18u) | (1u << 10u) | (1u << 9u));

    // TODO: Use xsaveopt if available.
    uint64_t xcr = 0b11u;
    if ((sse_cpu_id.ecx() & (1u << 28u)) != 0) {
        // AVX available.
        xcr |= 1u << 2u;
    }
    write_xcr(xcr);
    s_simd_region_size = CpuId(0xd).ecx();
    s_simd_default_region = new (ustd::align_val_t(64)) uint8_t[s_simd_region_size];
    asm volatile("xsave %0" : "=m"(*s_simd_default_region) : "a"(0xffffffffu), "d"(0xffffffffu));
}

void Processor::setup(uint8_t id) {
    auto *gdt = new Gdt;
    auto *tss = new Tss;

    // Set up a flat GDT.
    gdt->set(1, GlobalDescriptor::create_kernel(DescriptorType::Code));
    gdt->set(2, GlobalDescriptor::create_kernel(DescriptorType::Data));
    gdt->set(3, GlobalDescriptor::create_user(DescriptorType::Data));
    gdt->set(4, GlobalDescriptor::create_user(DescriptorType::Code));
    gdt->set(5, GlobalDescriptor::create_tss(reinterpret_cast<uintptr_t>(tss), sizeof(Tss) - 1));
    gdt->set(6, GlobalDescriptor::create_base_extended(reinterpret_cast<uintptr_t>(tss) >> 32u));

    // Set up the TSS by allocating a stack for interrupt handling and setting the IO permission bitmap base to
    // `sizeof(Tss)`. This makes any ring 3 attempt to use IO ports fail with a #GP exception since the TSS limit is
    // also its size.
    tss->rsp0 = tss->ist1 = new char[8_KiB] + 8_KiB;
    tss->ist1 = tss->rsp0;
    tss->iopb = sizeof(Tss);

    // Load our new GDT and TSS. Flushing the GDT involves performing an iret to change the current code selector.
    flush_gdt(gdt);
    asm volatile("ltr %%ax" : : "a"(0x28));

    // Allocate and store our CPU local storage struct.
    auto *storage = new LocalStorage;
    storage->id = id;
    write_msr(k_msr_gs_base, reinterpret_cast<uintptr_t>(storage));

    // Write selectors to the STAR MSR. First write the sysret CS and SS (63:48), then write the syscall CS and SS
    // (47:32). The bottom 32 bits for the target EIP are not used in long mode. 0x13 is used for the sysret CS/SS pair
    // because CS is set to 0x13+16 (0x23), which is the user code segment and SS is set to 0x13+8 (0x1b) which is the
    // user data segment. This is also the reason user code comes after user data in the GDT. 0x08 is used for the
    // syscall CS/SS pair because CS is set to 0x08, which is the kernel code segment and SS is set to 0x08+8 (0x10),
    // which is the kernel data segment.
    uint64_t star = 0;
    star |= 0x13ul << 48u;
    star |= 0x08ul << 32u;
    write_msr(k_msr_star, star);

    // Write the syscall entry point to the LSTAR MSR. Also set the CPU to clear rflags when a syscall occurs. The
    // interrupted rflags will be preserved in r11 by the CPU.
    write_msr(k_msr_lstar, reinterpret_cast<uintptr_t>(&syscall_stub));
    write_msr(k_msr_sfmask, 0x257fd5u);
}

void Processor::start_aps(acpi::ApicTable *madt) {
    const size_t bootstrap_size =
        reinterpret_cast<uintptr_t>(&ap_bootstrap_end) - reinterpret_cast<uintptr_t>(&ap_bootstrap);
    ENSURE(bootstrap_size <= 4_KiB);
    __builtin_memcpy(reinterpret_cast<void *>(0x8000),
                     reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(&ap_bootstrap)), bootstrap_size);
    ustd::ScopeGuard free_bootstrap_guard([] {
        MemoryManager::free_frame(0x8000);
    });

    uint8_t ap_count = 0;
    for (auto *controller : *madt) {
        if (controller->type == 0) {
            auto *local_apic = reinterpret_cast<acpi::LocalApicController *>(controller);
            if ((local_apic->flags & 1u) == 0 && (local_apic->flags & 2u) == 0) {
                dmesg(" cpu: CPU {} not available!", local_apic->apic_id);
                continue;
            }
            ap_count++;
        }
    }
    ENSURE(ap_count > 0);
    ap_count--;

    auto *stacks = ap_prepare();
    for (uint8_t i = 0; i < ap_count; i++) {
        // TODO: Free these stacks later on somehow.
        stacks[i] = MemoryManager::alloc_frame() + 4_KiB;
    }

    apic()->send_ipi(0, MessageType::Init, DestinationMode::Physical, Level::Assert, TriggerMode::Edge,
                     DestinationShorthand::AllExcludingSelf);
    TimeManager::spin(10);
    for (uint8_t i = 0; i < 2; i++) {
        apic()->send_ipi(8, MessageType::Startup, DestinationMode::Physical, Level::Assert, TriggerMode::Edge,
                         DestinationShorthand::AllExcludingSelf);
        TimeManager::spin(1);
    }

    while (s_initialised_ap_count.load(ustd::MemoryOrder::Acquire) != ap_count) {
        asm volatile("pause");
    }
    dmesg(" cpu: Started {} APs", ap_count);
}

void Processor::set_apic(LocalApic *apic) {
    ASSERT(s_apic == nullptr);
    s_apic = apic;
}

void Processor::set_current_space(VirtSpace *space) {
    write_gs(__builtin_offsetof(LocalStorage, current_space), reinterpret_cast<uint64_t>(space));
}

void Processor::set_current_thread(Thread *thread) {
    write_gs(__builtin_offsetof(LocalStorage, current_thread), reinterpret_cast<uint64_t>(thread));
}

void Processor::set_kernel_stack(void *stack) {
    write_gs(__builtin_offsetof(LocalStorage, kernel_stack), reinterpret_cast<uint64_t>(stack));
}

void Processor::wire_interrupt(uint64_t vector, InterruptHandler handler) {
    ASSERT(vector < k_interrupt_count);
    ASSERT(handler != nullptr);
    s_interrupt_table[vector] = handler;
}

void Processor::write_cr3(void *pml4) {
    // Make sure both the upper and lower 12 bits of the PML4 base address are clear. The upper 12 bits of CR3 should be
    // clear since the minimum physical address width required to be supported by the CPU is 40 bits. The lower 12 bits
    // of CR3 are used for flags and should be clear in the PML4 base address since it should be page-aligned.
    ASSERT_PEDANTIC((reinterpret_cast<uintptr_t>(pml4) & 0xfff0000000000fffu) == 0);
    asm volatile("mov %0, %%cr3" : : "r"(pml4) : "memory");
}

LocalApic *Processor::apic() {
    ASSERT_PEDANTIC(s_apic != nullptr);
    return s_apic;
}

VirtSpace *Processor::current_space() {
    return reinterpret_cast<VirtSpace *>(read_gs(__builtin_offsetof(LocalStorage, current_space)));
}

Thread *Processor::current_thread() {
    return reinterpret_cast<Thread *>(read_gs(__builtin_offsetof(LocalStorage, current_thread)));
}

uint8_t Processor::id() {
    return static_cast<uint8_t>(read_gs(__builtin_offsetof(LocalStorage, id)));
}

uint8_t *Processor::simd_default_region() {
    ASSERT(s_simd_default_region != nullptr);
    return s_simd_default_region;
}

uint32_t Processor::simd_region_size() {
    ASSERT(s_simd_region_size != 0u);
    return s_simd_region_size;
}

} // namespace kernel
