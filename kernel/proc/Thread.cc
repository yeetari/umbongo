#include <kernel/proc/Thread.hh>

#include <elf/Elf.hh>
#include <kernel/Dmesg.hh>
#include <kernel/SysResult.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/mem/VmObject.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/Numeric.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/ScopeGuard.hh> // IWYU pragma: keep
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

extern uint8_t k_user_access_start;
extern uint8_t k_user_access_end;
extern "C" bool user_copy(uintptr_t dst, uintptr_t src, size_t size);
extern "C" bool user_copy_fault();

namespace kernel {
namespace {

constexpr size_t k_kernel_stack_size = 8_KiB;

void dump_backtrace([[maybe_unused]] RegisterState *regs) {
    // TODO(GH-9): Backtrace generation is unsafe.
#ifndef ASSERTIONS
    auto *rbp = reinterpret_cast<uint64_t *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64_t *>(*rbp);
    }
#endif
}

} // namespace

ustd::UniquePtr<Thread> Thread::create_kernel(uintptr_t entry_point, ThreadPriority priority, ustd::String &&name) {
    auto *process = new Process(true, ustd::move(name), ustd::SharedPtr<VirtSpace>(MemoryManager::kernel_space()));
    auto thread = process->create_thread(priority);
    thread->m_register_state.rip = entry_point;
    return thread;
}

ustd::UniquePtr<Thread> Thread::create_user(ThreadPriority priority, ustd::String &&name) {
    auto *process = new Process(false, ustd::move(name), MemoryManager::kernel_space()->clone());
    return process->create_thread(priority);
}

void Thread::kill_and_yield() {
    Processor::current_thread()->set_state(ThreadState::Dead);
    Scheduler::yield(false);
}

Thread::Thread(Process *process, ThreadPriority priority) : m_process(process), m_priority(priority) {
    process->m_thread_count.fetch_add(1, ustd::MemoryOrder::AcqRel);
    m_kernel_stack = new uint8_t[k_kernel_stack_size] + k_kernel_stack_size;
    m_simd_region = new (ustd::align_val_t(64)) uint8_t[Processor::simd_region_size()];
    __builtin_memcpy(m_simd_region, Processor::simd_default_region(), Processor::simd_region_size());
    m_register_state.cs = process->m_is_kernel ? 0x08 : (0x20u | 0x3u);
    m_register_state.ss = process->m_is_kernel ? 0x10 : (0x18u | 0x3u);
    m_register_state.rflags = 0x202;
    if (process->m_is_kernel) {
        // Disable preemption for kernel threads.
        m_register_state.rflags &= ~(1u << 9u);

        // A kernel process doesn't use syscalls, so we can use m_kernel_stack.
        m_register_state.rsp = reinterpret_cast<uintptr_t>(m_kernel_stack);
    }
}

Thread::~Thread() {
    delete[] (m_kernel_stack - k_kernel_stack_size);
    operator delete[](m_simd_region, ustd::align_val_t(64));
    m_process->m_thread_count.fetch_sub(1, ustd::MemoryOrder::AcqRel);
    if (m_prev != nullptr) {
        ASSERT(m_next != nullptr);
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
    }
}

bool Thread::copy_from_user(void *dst, uintptr_t src, size_t size) {
    return src <= 256_GiB || user_copy(reinterpret_cast<uintptr_t>(dst), src, size);
}

bool Thread::copy_to_user(uintptr_t dst, void *src, size_t size) {
    return dst <= 256_GiB || user_copy(dst, reinterpret_cast<uintptr_t>(src), size);
}

// TODO: Move loading to a userspace library and or service, kernel only needs to load root program.
SysResult<> Thread::exec(UserPtr<const uint8_t> binary, size_t binary_size) {
    if (binary_size < sizeof(elf::Header)) {
        return Error::NoExec;
    }

    elf::Header header{};
    TRY(copy_from_user(&header, binary, sizeof(elf::Header)));

    if (!ustd::equal(header.magic, elf::k_magic)) {
        return Error::NoExec;
    }
    if (header.type != elf::ObjectType::Dyn) {
        return Error::NoExec;
    }
    if (header.ph_count > 256) {
        return Error::NoExec;
    }
    if (header.ph_size != sizeof(elf::ProgramHeader)) {
        return Error::NoExec;
    }
    if (header.ph_off + header.ph_count * sizeof(elf::ProgramHeader) > binary_size) {
        return Error::NoExec;
    }

    ustd::Vector<elf::ProgramHeader> phdrs(header.ph_count);
    TRY(copy_from_user(phdrs.data(), binary.offseted(header.ph_off), phdrs.size_bytes()));

    uintptr_t memory_base = ustd::Limits<uintptr_t>::max();
    uintptr_t memory_end = 0;
    for (const auto &phdr : phdrs) {
        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }
        if (phdr.filesz > phdr.memsz) {
            return Error::NoExec;
        }
        memory_base = ustd::min(memory_base, phdr.vaddr);
        memory_end = ustd::max(memory_end, phdr.vaddr + phdr.memsz);
    }

    // Immediately allocate and free some VM space to reserve a contigious block we can allocate it.
    auto executable_base =
        TRY(MemoryManager::current_space()->allocate_anywhere(RegionAccess::None, memory_end - memory_base))->base();



//    auto executable_vmo = VmObject::create(memory_end - memory_base);
//    auto copy_region = TRY(MemoryManager::current_space()->allocate_anywhere(RegionAccess::Writable, executable_vmo));

    size_t dynamic_entry_count = 0;
    size_t dynamic_table_offset = 0;
    for (const auto &phdr : phdrs) {
        if (phdr.type == elf::SegmentType::Dynamic) {
            if (phdr.filesz % sizeof(elf::DynamicEntry) != 0) {
                return Error::NoExec;
            }
            dynamic_entry_count = phdr.filesz / sizeof(elf::DynamicEntry);
            dynamic_table_offset = phdr.offset;
        }
    }

    ustd::Vector<elf::DynamicEntry> dynamic_entries(dynamic_entry_count);
    TRY(copy_from_user(dynamic_entries.data(), binary.offseted(dynamic_table_offset), dynamic_entries.size_bytes()));

    size_t relocation_entry_size = 0;
    size_t relocation_table_offset = 0;
    size_t relocation_table_size = 0;
    for (const auto &entry : dynamic_entries) {
        switch (entry.type) {
        case elf::DynamicType::Null:
        case elf::DynamicType::Hash:
        case elf::DynamicType::StrTab:
        case elf::DynamicType::SymTab:
        case elf::DynamicType::StrSz:
        case elf::DynamicType::SymEnt:
        case elf::DynamicType::Debug:
        case elf::DynamicType::GnuHash:
        case elf::DynamicType::RelaCount:
        case elf::DynamicType::Flags1:
            break;
        case elf::DynamicType::Rela:
            relocation_table_offset = entry.value;
            break;
        case elf::DynamicType::RelaSz:
            relocation_table_size = entry.value;
            break;
        case elf::DynamicType::RelaEnt:
            relocation_entry_size = entry.value;
            break;
        default:
            dmesg("Unknown dynamic entry type {}", ustd::to_underlying(entry.type));
            return Error::NoExec;
        }
    }

    for (uint16_t i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        TRY(copy_from_user(&phdr, binary.offseted(header.ph_off + i * sizeof(elf::ProgramHeader)),
                           sizeof(elf::ProgramHeader)));

        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }

        if (phdr.filesz > phdr.memsz) {
            return Error::NoExec;
        }

        auto vm_object = VmObject::create(phdr.memsz + (phdr.vaddr & 0xfffu));
        auto copy_region = TRY(MemoryManager::current_space()->allocate_anywhere(RegionAccess::Writable, vm_object));

        size_t copy_offset = phdr.vaddr & 0xfffu;
        ASSERT(copy_offset + phdr.memsz <= copy_region->range().size());

        auto *copy_dst = reinterpret_cast<uint8_t *>(copy_region->base() + copy_offset);
        if (phdr.filesz != phdr.memsz) {
            ustd::fill_n(copy_dst + phdr.filesz, phdr.memsz - phdr.filesz, 0);
        }
        TRY(copy_from_user(copy_dst, binary.offseted(phdr.offset), phdr.filesz));

        auto access = RegionAccess::UserAccessible;
        if ((phdr.flags & elf::SegmentFlags::Executable) == elf::SegmentFlags::Executable) {
            access |= RegionAccess::Executable;
        }
        if ((phdr.flags & elf::SegmentFlags::Writable) == elf::SegmentFlags::Writable) {
            access |= RegionAccess::Writable;
        }

        auto region = TRY(m_process->virt_space().allocate_anywhere(access, ustd::move(vm_object)));
        m_executable_regions.push(region);

        // Segment contains entry point.
        if (header.entry >= phdr.vaddr && header.entry < phdr.vaddr + phdr.memsz) {
            m_register_state.rip = region->base() + header.entry - (phdr.vaddr & ~4095u);
        }
    }

    auto stack_region = TRY(m_process->virt_space().allocate_anywhere(
        RegionAccess::Writable | RegionAccess::UserAccessible, VmObject::create(1_MiB)));
    m_executable_regions.push(stack_region);
    m_register_state.rsp = stack_region->range().end();

    // Allocate some space for heap storage.
    // TODO: Remove.
    auto heap_region = TRY(m_process->virt_space().allocate_specific(
        6_TiB, RegionAccess::Writable | RegionAccess::UserAccessible, VmObject::create(2_MiB)));
    m_executable_regions.push(heap_region);
    return {};
}

void Thread::handle_fault(RegisterState *regs) {
    const bool kernel_fault = (regs->cs & 3u) == 0u;
    const auto *rip = reinterpret_cast<uint8_t *>(regs->rip);
    if (kernel_fault && regs->int_num == 14 && rip >= &k_user_access_start && rip < &k_user_access_end) {
        // Fault in user copy.
        regs->rip = reinterpret_cast<uintptr_t>(&user_copy_fault);
        return;
    }

    uint64_t cr2 = 0;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));

    if (!kernel_fault) {
        dmesg("[#{}]: Fault {} caused by instruction at {:h}! (cr2={:h})", m_process->pid(), regs->int_num, regs->rip,
              cr2);
    }
    if (kernel_fault) {
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }
    dump_backtrace(regs);
    set_state(ThreadState::Dead);
    Scheduler::switch_next(regs);
}

void Thread::set_state(ThreadState state) {
    m_state.store(state);
}

void Thread::unblock() {
    ASSERT(state() == ThreadState::Blocked);
    set_state(ThreadState::Alive);
    Scheduler::requeue_thread(this);
}

SyscallResult Thread::sys_debug_line(UserPtr<const char> msg, size_t msg_len) {
    if (msg_len > 2048) {
        return Error::TooBig;
    }
    ustd::String string(msg_len);
    TRY(copy_from_user(string.data(), msg, msg_len));

    const auto name = m_process->name();
    dmesg("[{}{}#{}]: {}", name, !name.empty() ? " " : "", m_process->pid(), string);
    return 0;
}

SyscallResult Thread::sys_thrd_yield() {
    Scheduler::yield(true);
    return 0;
}

} // namespace kernel
