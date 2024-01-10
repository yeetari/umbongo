#include <kernel/arch/cpu.hh>

#include <kernel/acpi/apic_table.hh>
#include <kernel/acpi/root_table.hh>
#include <kernel/arch/amd64/cpu_id.hh>
#include <kernel/arch/amd64/descriptor.hh>
#include <kernel/arch/amd64/register_state.hh>
#include <kernel/dmesg.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/memory_manager.hh>
#include <kernel/proc/process.hh>
#include <kernel/proc/scheduler.hh>
#include <kernel/proc/thread.hh>
#include <kernel/time/time_manager.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/atomic.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

// Copied from linux to prevent reads from being reordered w.r.t. writes but still allowing memory loads to be cached by
// not requiring a full memory clobber.
#define FORCE_ORDER "m"(*(unsigned int *)0x1000UL)

// Defined in linker script.
extern uint8_t k_interrupt_stubs_start;
extern uint8_t k_interrupt_stubs_end;

namespace kernel::arch {
namespace {

// A struct to store per core information.
struct CpuStorage {
    CpuStorage *self{this};
    void *kernel_stack{nullptr};
    void *user_stack{nullptr};
    Thread *current_thread{nullptr};
    Gdt gdt{};
    Tss tss{};
    uint32_t apic_id{0};
    uint32_t index{0};

    static CpuStorage &current();
    static CpuStorage &from_index(uint32_t index);
};

// NOLINTBEGIN(*-cstyle-cast, *-init-variables)
#define GENERATE_CR(crn)                                                                                               \
    uint64_t read_##crn() {                                                                                            \
        uint64_t value;                                                                                                \
        asm volatile("mov %%" #crn ", %0" : "=r"(value) : FORCE_ORDER);                                                \
        return value;                                                                                                  \
    }                                                                                                                  \
                                                                                                                       \
    void write_##crn(uint64_t value) {                                                                                 \
        asm volatile("mov %0, %%" #crn : : "r"(value) : "memory");                                                     \
    }

GENERATE_CR(cr0)
GENERATE_CR(cr2)
GENERATE_CR(cr3)
GENERATE_CR(cr4)
// NOLINTEND(*-cstyle-cast, *-init-variables)

uint64_t read_msr(uint32_t msr) {
    uint32_t eax = 0;
    uint32_t edx = 0;
    asm volatile("rdmsr" : "=a"(eax), "=d"(edx) : "c"(msr));
    return (static_cast<uint64_t>(edx) << 32u) | eax;
}

void write_msr(uint32_t msr, uint64_t value) {
    const uint32_t eax = value & 0xffffffffu;
    const uint32_t edx = (value >> 32u) & 0xffffffffu;
    asm volatile("wrmsr" : : "a"(eax), "d"(edx), "c"(msr));
}

void write_pml4(uintptr_t pml4) {
    // Make sure both the upper and lower 12 bits of the PML4 base address are clear. The upper 12 bits of CR3 should be
    // clear since the minimum physical address width required to be supported by the CPU is 40 bits. The lower 12 bits
    // of CR3 are used for flags and should be clear in the PML4 base address since it should be page-aligned.
    ASSERT((pml4 & 0xfff0000000000fffu) == 0);
    write_cr3(pml4);
}

void write_port(uint16_t port, uint8_t value) {
    asm volatile("out %0, %1" : : "a"(value), "Nd"(port));
}

void write_xcr0(uint64_t value) {
    const uint32_t eax = value & 0xffffffffu;
    const uint32_t edx = (value >> 32u) & 0xffffffffu;
    asm volatile("xsetbv" : : "a"(eax), "d"(edx), "c"(0));
}

class IpiDestination {
    uint32_t m_destination;
    uint8_t m_shorthand;

    IpiDestination(uint32_t destination, uint8_t shorthand) : m_destination(destination), m_shorthand(shorthand) {}

public:
    static IpiDestination all_including_self() { return {0, 0b10}; }
    static IpiDestination all_excluding_self() { return {0, 0b11}; }
    static IpiDestination make_physical(uint32_t apic_id) { return {apic_id, 0b00}; }

    uint64_t value() const {
        return (static_cast<uint64_t>(m_destination) << 32u) | (static_cast<uint64_t>(m_shorthand) << 18u);
    }
};

enum class IpiType : uint32_t {
    Fixed = 0b000,
    Nmi = 0b100,
    Init = 0b101,
    Startup = 0b110,
};

void send_ipi(uint8_t vector, IpiType type, IpiDestination destination) {
    uint64_t message = vector;
    message |= ustd::to_underlying(type) << 8u;
    message |= 1u << 14u; // Assert level
    message |= destination.value();
    write_msr(0x830, message);
}

ustd::Array<InterruptHandler, 256> s_interrupt_table;
CpuStorage *s_cpu_storage_list = nullptr;
Idt *s_idt = nullptr;

// TODO: Support more than 64 CPUs.
ustd::Atomic<uint64_t, ustd::memory_order_seq_cst> s_online_cpu_set;
ustd::Atomic<uint32_t> s_total_cpu_count;
uint32_t s_ticks_in_one_ms = 0;
uint8_t *s_simd_default_region = nullptr;
uint32_t s_simd_region_size = 0;
bool s_smap_available = false;
bool s_smep_available = false;
bool s_umip_available = false;
bool s_bsp_initialised = false;

CpuStorage &CpuStorage::current() {
    CpuStorage *value = nullptr;
    asm volatile("mov %%gs:0, %0" : "=r"(value));
    return *value;
}

CpuStorage &CpuStorage::from_index(uint32_t index) {
    return s_cpu_storage_list[index];
}

extern "C" void ap_trampoline();
extern "C" void ap_trampoline_end();
extern "C" void load_gdt(Gdt *gdt);
extern "C" uintptr_t *smp_prepare();
extern "C" void syscall_entry();

[[noreturn]] void unhandled_interrupt(RegisterState *regs) {
    dmesg(" cpu: Received unexpected interrupt {} in ring {}!", regs->int_num, regs->cs & 3u);
    ENSURE_NOT_REACHED("Unhandled interrupt!");
}

void handle_fault(RegisterState *regs) {
    if ((regs->cs & 0b11u) == 0u) {
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }

    auto &thread = Thread::current();
    dmesg("CRASH! Process #{} faulted on CPU {} with exception {} (code: {})", thread.process().pid(),
          CpuStorage::current().index, regs->int_num, regs->error_code);
    dmesg("  rip={:p} rflags={:p}", regs->rip, regs->rflags);
    dmesg("  rax={:p} rbx={:p} rcx={:p} rdx={:p}", regs->rax, regs->rbx, regs->rcx, regs->rdx);
    dmesg("  rdi={:p} rsi={:p} rbp={:p} rsp={:p}", regs->rdi, regs->rsi, regs->rbp, regs->rsp);
    dmesg("   r8={:p}  r9={:p} r10={:p} r11={:p}", regs->r8, regs->r9, regs->r10, regs->r11);
    dmesg("  r12={:p} r13={:p} r14={:p} r15={:p}", regs->r12, regs->r13, regs->r14, regs->r15);
    dmesg("   fs={:p}  gs={:p}", read_msr(0xc0000100), read_msr(0xc0000101));
    dmesg("  cr0={:p} cr2={:p} cr3={:p} cr4={:p}", read_cr0(), read_cr2(), read_cr3(), read_cr4());

    thread.kill();
    Scheduler::switch_next(regs);
}

void halt_cpu(RegisterState *regs) {
    auto &cpu_storage = CpuStorage::current();

    // Check if already halted; this should only happen if we are interrupted by an NMI.
    if ((s_online_cpu_set.load() & (1ull << cpu_storage.index)) == 0u) {
        return;
    }

    if (regs != nullptr && regs->int_num == 2) {
        dmesg("Force halting due to NMI!");
    }

    // Soft disable the local APIC.
    write_msr(0x80f, read_msr(0x80f) & ~(1u << 8u));

    // Mark ourselves halted.
    s_online_cpu_set.fetch_and(~(1ull << cpu_storage.index));

    // Halt indefinitely.
    while (true) {
        asm volatile("hlt" ::: "memory");
    }
}

bool try_broadcast_halt() {
    // First, try to send a regular halt interrupt.
    send_ipi(0xf8, IpiType::Fixed, IpiDestination::all_excluding_self());

    // Wait up to ~500 ms for CPUs to halt. Use an IO delay since we can't rely on the HPET still working.
    for (uint32_t i = 0; i < 500000; i++) {
        if (ustd::popcount(s_online_cpu_set.load()) == 1) {
            return true;
        }
        write_port(0x80, 0);
    }

    // Not all CPUs halted, send an NMI instead.
    uint64_t online_set = s_online_cpu_set.load() & ~(1ull << CpuStorage::current().index);
    while (online_set != 0) {
        uint32_t ap_index = ustd::ctz(online_set);
        online_set &= ~(1ull << ap_index);

        auto &cpu_storage = CpuStorage::from_index(ap_index);
        send_ipi(0, IpiType::Nmi, IpiDestination::make_physical(cpu_storage.apic_id));
    }

    // Wait up to ~10 ms.
    for (uint32_t i = 0; i < 10000; i++) {
        if (ustd::popcount(s_online_cpu_set.load()) == 1) {
            return true;
        }
        write_port(0x80, 0);
    }
    return ustd::popcount(s_online_cpu_set.load()) == 1;
}

void setup_cpu(uint32_t index) {
    // Load the shared IDT.
    ASSERT(s_idt != nullptr);
    asm volatile("lidt %0" : : "m"(*s_idt));

    // Create and store the address of our CPU local storage struct in the GS base MSR.
    auto *cpu_storage = new (&s_cpu_storage_list[index]) CpuStorage{
        .index = index,
    };
    write_msr(0xc0000101, ustd::bit_cast<uintptr_t>(cpu_storage));

    // Allocate a stack to be loaded by the CPU when there is a privilege level change (ring 3 -> ring 0), which is
    // *almost* always the case since we disable interrupt nesting (IF is always 0 in ring 0). The exception is
    // non-maskable interrupts (NMIs).
    cpu_storage->tss.rsp0 = new char[8_KiB] + 8_KiB;

    // For the NMI case, we use the IST switching mechanism.
    cpu_storage->tss.ist1 = new char[4_KiB] + 4_KiB;

    // Prevent userspace from using IO ports.
    cpu_storage->tss.iopb = sizeof(Tss);

    // Set up a flat GDT.
    const auto tss_address = ustd::bit_cast<uintptr_t>(&cpu_storage->tss);
    cpu_storage->gdt.set(1, GlobalDescriptor::create_kernel(DescriptorType::Code));
    cpu_storage->gdt.set(2, GlobalDescriptor::create_kernel(DescriptorType::Data));
    cpu_storage->gdt.set(3, GlobalDescriptor::create_user(DescriptorType::Data));
    cpu_storage->gdt.set(4, GlobalDescriptor::create_user(DescriptorType::Code));
    cpu_storage->gdt.set(5, GlobalDescriptor::create_tss(tss_address, sizeof(Tss) - 1));
    cpu_storage->gdt.set(6, GlobalDescriptor::create_base_extended(tss_address >> 32u));

    // Load our new GDT and TSS.
    load_gdt(&cpu_storage->gdt);

    // Enable the APIC (xAPIC mode).
    write_msr(0x1b, read_msr(0x1b) | (1u << 11u));

    // Then enable x2APIC mode.
    write_msr(0x1b, read_msr(0x1b) | (1u << 10u));

    // Set the spurious interrupt vector to 255 and set the APIC software enable bit.
    write_msr(0x80f, (1u << 8u) | 255u);

    // Acknowledge any outstanding stale interrupts.
    write_msr(0x80b, 0);

    // Configure the APIC timer to interrupt vector 0xec with a 16x divisor in one-shot mode.
    write_msr(0x83e, 0b11);
    write_msr(0x832, 0xec);

    // Store our x2APIC id.
    cpu_storage->apic_id = read_msr(0x802) & 0xffffffffu;

    // Configure CR0 feature bits.
    uint64_t cr0 = read_cr0();
    cr0 |= 1u << 1u;     // Set MP
    cr0 &= ~(1u << 2u);  // Unset EM; disable x87 software emulation
    cr0 |= 1u << 5u;     // Set NE; generate internal FP exceptions
    cr0 |= 1u << 16u;    // Set WP; prevent the kernel from writing to read-only pages
    cr0 &= ~(1u << 30u); // Unset CD; make sure caches are enabled
    write_cr0(cr0);

    // Configure CR4 feature bits.
    uint64_t cr4 = read_cr4();
    cr4 |= 1u << 7u;  // Set PGE; enable support for global pages
    cr4 |= 1u << 9u;  // Set OSFXSR; enable legacy SSE instructions
    cr4 |= 1u << 10u; // Set OSXMMEXCPT; enable use of #XF exception
    if (s_umip_available) {
        cr4 |= 1u << 11u; // Set UMIP; disable sgdt and sidt in ring 3
    }
    cr4 &= ~(1u << 16u); // Unset FSGSBASE; disable use of writing FS/GS base in ring 3
    cr4 |= 1u << 18u;    // Set OSXSAVE; enable use of xsave, xrstor, xgetbv, xsetbv
    if (s_smep_available) {
        cr4 |= 1u << 20u; // Set SMEP; prevent the kernel from executing user-accessible code
    }
    if (s_smap_available) {
        // TODO: Enable SMAP (bit 21).
    }
    write_cr4(cr4);

    // Configure EFER feature bits.
    uint64_t efer = read_msr(0xc0000080);
    efer |= 1u << 0u;  // Set SCE; enable use of syscall/sysret
    efer |= 1u << 11u; // Set NXE; enable no-execute page flag
    write_msr(0xc0000080, efer);

    // Enable xsave x87, 128-bit, and 256-bit (if available) state saving.
    // TODO: Use xsaveopt if available.
    const auto xcr_supported = CpuId(0xd).eax();
    uint64_t xcr_value = 0b11u;
    if ((xcr_supported & (1u << 2u)) != 0u) {
        xcr_value |= 1u << 2u;
    }
    write_xcr0(xcr_value);

    // Write selectors to the STAR MSR. First write the sysret CS and SS (63:48), then write the syscall CS and SS
    // (47:32). The bottom 32 bits for the target EIP are not used in long mode. 0x13 is used for the sysret CS/SS pair
    // because CS is set to 0x13+16 (0x23), which is the user code segment and SS is set to 0x13+8 (0x1b) which is the
    // user data segment. This is also the reason user code comes after user data in the GDT. 0x08 is used for the
    // syscall CS/SS pair because CS is set to 0x08, which is the kernel code segment and SS is set to 0x08+8 (0x10),
    // which is the kernel data segment.
    uint64_t star = 0;
    star |= 0x13ul << 48u;
    star |= 0x08ul << 32u;
    write_msr(0xc0000081, star);

    // Write the syscall entry point to the LSTAR MSR. Also set the CPU to clear rflags when a syscall occurs. The
    // interrupted rflags will be preserved in r11 by the CPU.
    write_msr(0xc0000082, ustd::bit_cast<uintptr_t>(&syscall_entry));
    write_msr(0xc0000084, 0x257fd5u);

    // Mark ourselves as online.
    s_online_cpu_set.fetch_or(1ull << index);
}

bool try_start_ap(uint32_t apic_id) {
    // Send an init IPI to the target AP.
    send_ipi(0, IpiType::Init, IpiDestination::make_physical(apic_id));

    // Wait for 10 ms.
    TimeManager::spin(10);

    // Wait up to 100 ms for the AP to come online.
    const uint32_t expected_cpu_count = s_total_cpu_count.load() + 1;
    for (uint32_t i = 0; i < 100; i++) {
        if (s_total_cpu_count.load() == expected_cpu_count) {
            return true;
        }
        if (i == 0 || i == 1) {
            // Send a SIPI to the target AP with a vector of 8 corresponding to a code start address of 0x8000.
            send_ipi(8, IpiType::Startup, IpiDestination::make_physical(apic_id));
        }
        TimeManager::spin(1);
    }
    return false;
}

} // namespace

extern "C" void ap_entry() {
    setup_cpu(s_total_cpu_count.fetch_add(1));
    sched_start(nullptr);
}

extern "C" void interrupt_handler(RegisterState *regs) {
    // Acknowledge interrupt if necessary.
    if (regs->int_num >= 32) {
        write_msr(0x80b, 0);
    }
    s_interrupt_table[regs->int_num](regs);
}

uint32_t current_cpu() {
    // TODO: Would probably be faster to read from GS directly.
    return CpuStorage::current().index;
}

void bsp_init(const acpi::RootTable *xsdt) {
    CpuId standard_features(0x1);
    if ((standard_features.ecx() & (1u << 21u)) == 0u) {
        ENSURE_NOT_REACHED("x2apic not available!");
    }
    if ((standard_features.ecx() & (1u << 26u)) == 0u) {
        ENSURE_NOT_REACHED("xsave not available!");
    }
    if ((standard_features.edx() & (1u << 13u)) == 0u) {
        ENSURE_NOT_REACHED("PGE not available!");
    }
    if ((standard_features.edx() & (1u << 25u)) == 0u) {
        ENSURE_NOT_REACHED("SSE1 not available!");
    }
    if ((standard_features.edx() & (1u << 26u)) == 0u) {
        ENSURE_NOT_REACHED("SSE2 not available!");
    }

    CpuId amd_extended_features(0x80000001);
    if ((amd_extended_features.edx() & (1u << 11u)) == 0u) {
        ENSURE_NOT_REACHED("syscall/sysret not available!");
    }
    if ((amd_extended_features.edx() & (1u << 20u)) == 0u) {
        ENSURE_NOT_REACHED("NX not available!");
    }
    if ((amd_extended_features.edx() & (1u << 26u)) == 0u) {
        ENSURE_NOT_REACHED("1 GiB pages not available!");
    }

    CpuId intel_extended_features(0x7);
    s_smap_available = (intel_extended_features.ebx() & (1u << 20u)) != 0u;
    s_smep_available = (intel_extended_features.ebx() & (1u << 7u)) != 0u;
    s_umip_available = (intel_extended_features.ecx() & (1u << 2u)) != 0u;

    // Mask legacy PIC if present.
    const auto *madt = EXPECT(xsdt->find<acpi::ApicTable>());
    if ((madt->flags() & 1u) != 0) {
        write_port(0x21, 0xff);
        write_port(0xa1, 0xff);
    }

    // Create and fill an IDT with our asm handler stubs. Also make sure that the interrupt stub section isn't empty,
    // which could happen if the linker accidentally removes it.
    ENSURE(&k_interrupt_stubs_start != &k_interrupt_stubs_end);
    s_idt = new Idt;
    for (auto *ptr = &k_interrupt_stubs_start; ptr < &k_interrupt_stubs_end; ptr += 12) {
        const uint64_t vector = static_cast<uint64_t>(ptr - &k_interrupt_stubs_start) / 12u;
        const uint8_t ist = vector == 2 ? 1 : 0;
        s_idt->set(vector, InterruptDescriptor::create_kernel(reinterpret_cast<void (*)()>(ptr), ist));
    }

    // Fill the interrupt handler table with trap handlers to ensure any unexpected interrupts are caught.
    ustd::fill(s_interrupt_table, &unhandled_interrupt);

    // Wire some exception handlers.
    wire_interrupt(0, handle_fault);  // Divide by zero
    wire_interrupt(1, handle_fault);  // Debug
    wire_interrupt(2, halt_cpu);      // NMI
    wire_interrupt(3, handle_fault);  // Breakpoint
    wire_interrupt(4, handle_fault);  // Overflow
    wire_interrupt(5, handle_fault);  // Bound range
    wire_interrupt(6, handle_fault);  // Invalid opcode
    wire_interrupt(13, handle_fault); // GP fault
    wire_interrupt(14, handle_fault); // Page fault
    wire_interrupt(17, handle_fault); // x87 FP exception
    wire_interrupt(19, handle_fault); // SIMD FP exception
    wire_interrupt(0xf8, halt_cpu);   // IPI halt interrupt

    // Very simple TLB flush handler.
    wire_interrupt(0xf7, [](RegisterState *) {
        // Writing to CR3 will flush the full TLB excluding global pages.
        write_cr3(read_cr3());
    });

    // Allocate for a maximum of 64 CPUs.
    s_cpu_storage_list = new CpuStorage[64];
    s_online_cpu_set.store(0);
    s_total_cpu_count.store(1);

    // Setup per-core data (BSP is index 0).
    setup_cpu(0);

    // Start the APIC timer counting down from its max value.
    write_msr(0x838, 0xffffffff);

    // Spin for 1 ms and then calculate the total number of ticks occured in that time by the APIC timer.
    TimeManager::spin(1);
    s_ticks_in_one_ms = 0xffffffff - read_msr(0x839);

    // Reinitialise FP state and save a default xsave region.
    s_simd_region_size = CpuId(0xd).ebx();
    s_simd_default_region = new (ustd::align_val_t(64)) uint8_t[s_simd_region_size];
    asm volatile("fninit");
    asm volatile("xsave %0" : "=m"(*s_simd_default_region) : "a"(0xffffffffu), "d"(0xffffffffu));

    s_bsp_initialised = true;
}

void smp_init(const acpi::RootTable *xsdt) {
    // Copy AP trampoline code and data to 0x8000.
    const size_t trampoline_size =
        reinterpret_cast<uintptr_t>(&ap_trampoline_end) - reinterpret_cast<uintptr_t>(&ap_trampoline);
    ENSURE(trampoline_size <= 4_KiB);
    __builtin_memcpy(reinterpret_cast<void *>(0x8000), reinterpret_cast<void *>(&ap_trampoline), trampoline_size);

    const uint32_t bsp_apic_id = read_msr(0x802) & 0xffffffffu;
    uintptr_t *stack_pointer = smp_prepare();

    // Start APs one by one.
    const auto *madt = EXPECT(xsdt->find<acpi::ApicTable>());
    for (auto *controller : *madt) {
        if (controller->type != 0) {
            continue;
        }
        const auto *local_apic = reinterpret_cast<acpi::LocalApicController *>(controller);
        if ((local_apic->flags & 1u) == 0 && (local_apic->flags & 2u) == 0) {
            // Processor neither enabled nor online capable.
            continue;
        }
        if (local_apic->apic_id == bsp_apic_id) {
            // Skip the BSP.
            continue;
        }

        if (s_total_cpu_count.load() == 64) {
            break;
        }

        // This stack will be kept alive and used for each AP's idle thread.
        *stack_pointer = MemoryManager::alloc_frame() + 4_KiB;
        try_start_ap(local_apic->apic_id);
    }

    // All APs started, we can now free the 0x8000 frame.
    MemoryManager::free_frame(0x8000);
}

[[noreturn]] void sched_start(Thread *base_thread) {
    // Each AP needs an idle thread created to ensure that there is always something to schedule.
    const bool is_ap = base_thread == nullptr;
    if (is_ap) {
        auto idle_thread = Thread::create_kernel(nullptr, ThreadPriority::Idle);
        CpuStorage::current().current_thread = idle_thread.ptr();
        Scheduler::insert_thread(ustd::move(idle_thread));
    } else {
        CpuStorage::current().current_thread = base_thread;

        // BSP needs a new stack since we are currently using the one provided by the bootloader.
        // TODO: This is a bit dodgy.
        asm volatile("mov %0, %%rsp" : : "r"(MemoryManager::alloc_frame() + 4_KiB) : "rsp");
    }

    // Start the timer.
    timer_set_one_shot(1);

    // Main idle loop.
    while (true) {
        asm volatile("sti; hlt" ::: "memory");
    }
}

void switch_space(AddressSpace &address_space) {
    const auto pml4_address = ustd::bit_cast<uintptr_t>(address_space.pml4_ptr());
    if (read_cr3() != pml4_address) {
        write_pml4(pml4_address);
    }
}

void thread_init(Thread *thread) {
    auto *simd_region = new (ustd::align_val_t(64)) uint8_t[s_simd_region_size];
    __builtin_memcpy(simd_region, s_simd_default_region, s_simd_region_size);
    thread->set_simd_region(simd_region);

    const bool is_kernel = thread->process().is_kernel();
    thread->register_state().cs = is_kernel ? 0x08u : (0x20u | 0x3u);
    thread->register_state().ss = is_kernel ? 0x10u : (0x18u | 0x3u);
    thread->register_state().rflags = 0x202u;
    if (is_kernel) {
        // Disable preemption for kernel threads.
        thread->register_state().rflags &= ~(1u << 9u);

        // A kernel process doesn't use syscalls, so we can use m_kernel_stack.
        thread->register_state().rsp = ustd::bit_cast<uintptr_t>(thread->kernel_stack());
    }
}

void thread_load(RegisterState *state, Thread *thread) {
    __builtin_memcpy(state, &thread->register_state(), sizeof(RegisterState));
    asm volatile("xrstor %0" : : "m"(*thread->simd_region()), "a"(0xffffffffu), "d"(0xffffffffu));

    auto &cpu_storage = CpuStorage::current();
    cpu_storage.current_thread = thread;
    cpu_storage.kernel_stack = thread->kernel_stack();
}

void thread_save(RegisterState *state) {
    auto &thread = Thread::current();
    __builtin_memcpy(&thread.register_state(), state, sizeof(RegisterState));
    asm volatile("xsave %0" : "=m"(*thread.simd_region()) : "a"(0xffffffffu), "d"(0xffffffffu));
}

void timer_set_one_shot(uint32_t ms) {
    // Set initial timer count.
    write_msr(0x838, ms * s_ticks_in_one_ms);
}

// TODO: Send these parameters to other CPUs with the IPI to avoid a full flush.
void tlb_flush_range(AddressSpace &address_space, VirtualRange range) {
    if (!s_bsp_initialised) {
        // We are really early in boot.
        return;
    }

    // If we're modfiying different page tables to our currently active ones, we don't need to flush anything on our
    // current CPU, but we do need to inform other CPUs if there are still threads running.
    auto &process = address_space.process();
    if (read_cr3() != ustd::bit_cast<uintptr_t>(address_space.pml4_ptr())) {
        if (process.thread_count() > 0) {
            // Broadcast flush IPI.
            send_ipi(0xf7, IpiType::Fixed, IpiDestination::all_excluding_self());
        }
        return;
    }

    // Check if other CPUs may be executing threads of the process.
    const bool is_running_a_thread = &process == &Thread::current().process();
    if ((!is_running_a_thread && process.thread_count() > 0) || process.thread_count() > 1) {
        // Broadcast flush IPI.
        send_ipi(0xf7, IpiType::Fixed, IpiDestination::all_excluding_self());
    }

    auto page_count = ustd::ceil_div(range.size(), 4_KiB);

    // Just do a full flush if there's a lot of pages, linux uses a threshold of 33 as the default value.
    if (page_count > 33) {
        write_cr3(read_cr3());
        return;
    }

    // Otherwise, invalidate in a loop.
    uintptr_t base = range.base();
    while (page_count-- > 0) {
        asm volatile("invlpg (%0)" : : "r"(base) : "memory");
        base += 4_KiB;
    }
}

void vm_debug_char(char ch) {
    write_port(0xe9, ch);
}

void wire_interrupt(uint8_t vector, InterruptHandler handler) {
    s_interrupt_table[vector] = handler;
}

void wire_timer(InterruptHandler handler) {
    wire_interrupt(0xec, handler);
}

} // namespace kernel::arch

namespace kernel {

// TODO: May be able to remove if we put syscall handlers on the thread.
Thread &Thread::current() {
    return *arch::CpuStorage::current().current_thread;
}

} // namespace kernel

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    using namespace kernel;

    dmesg_no_lock("\nKernel panic, halting all CPUs");
    if (!arch::try_broadcast_halt()) {
        dmesg_no_lock("Failed to halt all other CPUs");
    }

    dmesg_no_lock("Assertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        dmesg_no_lock("=> {}", msg);
    }

    // Halt ourselves.
    arch::halt_cpu(nullptr);
    ustd::unreachable();
}
