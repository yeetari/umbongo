#include <kernel/proc/Thread.hh>

#include <elf/Elf.hh>
#include <kernel/Dmesg.hh>
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/InterruptDisabler.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/ThreadBlocker.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <ustd/Assert.hh>
#include <ustd/Atomic.hh>
#include <ustd/Memory.hh>
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

namespace kernel {
namespace {

constexpr usize k_kernel_stack_size = 64_KiB;

void dump_backtrace([[maybe_unused]] RegisterState *regs) {
    // TODO(GH-9): Backtrace generation is unsafe.
#ifndef ASSERTIONS
    auto *rbp = reinterpret_cast<uint64 *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
#endif
}

ustd::Optional<ustd::String> interpreter_for(File &file) {
    elf::Header header{};
    if (file.read({&header, sizeof(elf::Header)}) != sizeof(elf::Header)) {
        return {};
    }
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        if (file.read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i) !=
            sizeof(elf::ProgramHeader)) {
            return {};
        }
        if (phdr.type == elf::SegmentType::Interp) {
            // TODO: Size limit.
            ustd::String path(phdr.filesz - 1);
            if (file.read({path.data(), phdr.filesz - 1}, phdr.offset) != phdr.filesz - 1) {
                return {};
            }
            return ustd::move(path);
        }
    }
    return {{}};
}

} // namespace

ustd::UniquePtr<Thread> Thread::create_kernel(uintptr entry_point, ThreadPriority priority) {
    auto *process = new Process(true, ustd::SharedPtr<VirtSpace>(MemoryManager::kernel_space()));
    auto thread = process->create_thread(priority);
    thread->m_register_state.rip = entry_point;
    return thread;
}

ustd::UniquePtr<Thread> Thread::create_user(ThreadPriority priority) {
    auto *process = new Process(false, MemoryManager::kernel_space()->clone());
    return process->create_thread(priority);
}

Thread::Thread(Process *process, ThreadPriority priority) : m_process(process), m_priority(priority) {
    process->m_thread_count.fetch_add(1, ustd::MemoryOrder::AcqRel);
    m_kernel_stack = new uint8[k_kernel_stack_size] + k_kernel_stack_size;
    m_simd_region = new (ustd::align_val_t(64)) uint8[Processor::simd_region_size()];
    __builtin_memcpy(m_simd_region, Processor::simd_default_region(), Processor::simd_region_size());
    m_register_state.cs = process->m_is_kernel ? 0x08 : (0x20u | 0x3u);
    m_register_state.ss = process->m_is_kernel ? 0x10 : (0x18u | 0x3u);
    m_register_state.rflags = 0x202;
    if (process->m_is_kernel) {
        // A kernel process doesn't use syscalls, so we can use m_kernel_stack.
        m_register_state.rsp = reinterpret_cast<uintptr>(m_kernel_stack);
    }
}

Thread::~Thread() {
    delete[](m_kernel_stack - k_kernel_stack_size);
    operator delete[](m_simd_region, ustd::align_val_t(64));
    m_process->m_thread_count.fetch_sub(1, ustd::MemoryOrder::AcqRel);
    if (m_prev != nullptr) {
        ASSERT(m_next != nullptr);
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
    }
}

SysResult<> Thread::exec(ustd::StringView path, const ustd::Vector<ustd::String> &args) {
    auto file = TRY(Vfs::open(path, OpenMode::None, m_process->m_cwd));
    auto &stack_region =
        m_process->m_virt_space->allocate_region(2_MiB, RegionAccess::Writable | RegionAccess::UserAccessible);
    m_register_state.rsp = stack_region.base() + stack_region.size();

    // We don't want to be interrupted whilst we have our virt space mapped in to copy the executable data.
    InterruptDisabler disabler;
    auto *original_space = MemoryManager::current_space();
    MemoryManager::switch_space(*m_process->m_virt_space);
    ustd::ScopeGuard space_guard([original_space] {
        MemoryManager::switch_space(*original_space);
    });

    auto interpreter_path = interpreter_for(*file);
    if (!interpreter_path) {
        // If there was an error trying to parse the interpreter location, not if the executable doesn't have one.
        return SysError::NoExec;
    }
    auto executable = file;
    if (!interpreter_path->empty()) {
        auto interpreter = TRY(Vfs::open(*interpreter_path, OpenMode::None, m_process->m_cwd));
        executable = ustd::move(interpreter);
    }

    elf::Header header{};
    if (executable->read({&header, sizeof(elf::Header)}) != sizeof(elf::Header)) {
        return SysError::NoExec;
    }
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        if (executable->read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i) !=
                sizeof(elf::ProgramHeader) ||
            phdr.filesz > phdr.memsz) {
            return SysError::NoExec;
        }
        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }
        auto access = RegionAccess::UserAccessible;
        if ((phdr.flags & elf::SegmentFlags::Executable) == elf::SegmentFlags::Executable) {
            access |= RegionAccess::Executable;
        }
        // TODO: Don't always map writable. We only need to for copying the data in.
        access |= RegionAccess::Writable;
        auto &region = m_process->m_virt_space->allocate_region(phdr.memsz + (phdr.vaddr & 0xfffu), access);

        // Segment contains entry point.
        if (header.entry >= phdr.vaddr && header.entry < phdr.vaddr + phdr.memsz) {
            m_register_state.rip = region.base() + header.entry - (phdr.vaddr & ~4095u);
        }

        usize copy_offset = phdr.vaddr & 0xfffu;
        if (copy_offset + phdr.memsz > region.size()) {
            return SysError::NoExec;
        }
        if (executable->read({reinterpret_cast<void *>(region.base() + copy_offset), phdr.filesz}, phdr.offset) !=
            phdr.filesz) {
            return SysError::NoExec;
        }
    }

    // Setup user stack.
    ustd::Vector<uintptr> argv;
    auto push_arg = [&](ustd::StringView arg) {
        m_register_state.rsp -= ustd::round_up(arg.length() + 1, sizeof(usize));
        __builtin_memcpy(reinterpret_cast<void *>(m_register_state.rsp), arg.data(), arg.length());
        *(reinterpret_cast<char *>(m_register_state.rsp) + arg.length()) = '\0';
        argv.push(m_register_state.rsp);
    };
    if (!interpreter_path->empty()) {
        push_arg(path);
    }
    for (const auto &arg : args) {
        push_arg(arg);
    }

    m_register_state.rsp -= sizeof(uintptr);
    *reinterpret_cast<uintptr *>(m_register_state.rsp) = 0;
    for (uint32 i = 0; i < argv.size(); i++) {
        m_register_state.rsp -= sizeof(uintptr);
        *reinterpret_cast<uintptr *>(m_register_state.rsp) = argv[argv.size() - i - 1];
    }

    m_register_state.rdi = argv.size();          // argc
    m_register_state.rsi = m_register_state.rsp; // argv

    // Allocate some space for heap storage.
    m_process->m_virt_space->create_region(6_TiB, 5_MiB, RegionAccess::Writable | RegionAccess::UserAccessible);
    return {};
}

void Thread::handle_fault(RegisterState *regs) {
    uint64 cr2 = 0;
    asm volatile("mov %%cr2, %0" : "=r"(cr2));
    if ((regs->cs & 3u) == 0u) {
        dmesg_unlock();
    }
    dmesg("[#{}]: Fault {} caused by instruction at {:h}! (cr2={:h})", m_process->pid(), regs->int_num, regs->rip, cr2);
    if ((regs->cs & 3u) == 0u) {
        ENSURE_NOT_REACHED("Fault in ring 0!");
    }
    dump_backtrace(regs);
    kill();
    Scheduler::switch_next(regs);
}

void Thread::kill() {
    m_state = ThreadState::Dead;
}

void Thread::try_unblock() {
    if (m_state != ThreadState::Blocked) {
        return;
    }
    ASSERT_PEDANTIC(m_blocker);
    if (m_blocker->should_unblock()) {
        m_blocker.clear();
        m_state = ThreadState::Alive;
    }
}

} // namespace kernel
