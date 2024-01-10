#include <kernel/proc/thread.hh>

#include <elf/elf.hh>
#include <kernel/api/types.h>
#include <kernel/arch/cpu.hh>
#include <kernel/dmesg.hh>
#include <kernel/error.hh>
#include <kernel/fs/file.hh>
#include <kernel/fs/vfs.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/memory_manager.hh>
#include <kernel/mem/region.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/proc/process.hh>
#include <kernel/proc/scheduler.hh>
#include <kernel/proc/thread_blocker.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/atomic.hh>
#include <ustd/numeric.hh>
#include <ustd/optional.hh>
#include <ustd/result.hh>
#include <ustd/scope_guard.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {
namespace {

constexpr size_t k_kernel_stack_size = 32_KiB;

void dump_backtrace([[maybe_unused]] arch::RegisterState *regs) {
    // TODO(GH-9): Backtrace generation is unsafe.
#ifndef ASSERTIONS
    auto *rbp = reinterpret_cast<uint64_t *>(regs->rbp);
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64_t *>(*rbp);
    }
#endif
}

ustd::Optional<ustd::String> interpreter_for(File &file) {
    elf::Header header{};
    if (file.read({&header, sizeof(elf::Header)}) != sizeof(elf::Header)) {
        return {};
    }
    for (uint16_t i = 0; i < header.ph_count; i++) {
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

ustd::UniquePtr<Thread> Thread::create_kernel(uintptr_t entry_point, ThreadPriority priority) {
    auto *process = new Process(true);
    auto thread = process->create_thread(priority);
    thread->m_register_state.rip = entry_point;
    return thread;
}

ustd::UniquePtr<Thread> Thread::create_user(ThreadPriority priority) {
    auto *process = new Process(false);
    return process->create_thread(priority);
}

Thread::Thread(Process *process, ThreadPriority priority) : m_process(process), m_priority(priority) {
    // Don't bother creating a kernel stack for idle threads since they can reuse their AP stack.
    if (priority != ThreadPriority::Idle) {
        m_kernel_stack = new uint8_t[k_kernel_stack_size] + k_kernel_stack_size;
    }
    process->m_thread_count.fetch_add(1, ustd::memory_order_acq_rel);
    arch::thread_init(this);
}

Thread::~Thread() {
    delete[] (m_kernel_stack - k_kernel_stack_size);
    operator delete[](m_simd_region, ustd::align_val_t(64));
    m_process->m_thread_count.fetch_sub(1, ustd::memory_order_acq_rel);
    if (m_prev != nullptr) {
        ASSERT(m_next != nullptr);
        m_prev->m_next = m_next;
        m_next->m_prev = m_prev;
    }
}

SysResult<> Thread::exec(ustd::StringView path, const ustd::Vector<ustd::String> &args) {
    auto file = TRY(Vfs::open(path, UB_OPEN_MODE_NONE, m_process->m_cwd));

    auto stack_object = VmObject::create(2_MiB);
    auto &stack_region = TRY(m_process->address_space().allocate_anywhere(
        stack_object->size(), RegionAccess::Writable | RegionAccess::UserAccessible));
    stack_region.map(ustd::move(stack_object));
    m_register_state.rsp = stack_region.range().end();

    arch::switch_space(m_process->address_space());
    ustd::ScopeGuard space_guard([] {
        arch::switch_space(Thread::current().process().address_space());
    });

    auto interpreter_path = interpreter_for(*file);
    if (!interpreter_path) {
        // If there was an error trying to parse the interpreter location, not if the executable doesn't have one.
        return Error::NoExec;
    }
    auto executable = file;
    if (!interpreter_path->empty()) {
        auto interpreter = TRY(Vfs::open(*interpreter_path, UB_OPEN_MODE_NONE, m_process->m_cwd));
        executable = ustd::move(interpreter);
    }

    elf::Header header{};
    if (executable->read({&header, sizeof(elf::Header)}) != sizeof(elf::Header)) {
        return Error::NoExec;
    }
    for (uint16_t i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        if (executable->read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i) !=
                sizeof(elf::ProgramHeader) ||
            phdr.filesz > phdr.memsz) {
            return Error::NoExec;
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

        auto vm_object = VmObject::create(phdr.memsz + (phdr.vaddr & 0xfffu));
        auto &region = TRY(m_process->address_space().allocate_anywhere(vm_object->size(), access));
        region.map(ustd::move(vm_object));

        // Segment contains entry point.
        if (header.entry >= phdr.vaddr && header.entry < phdr.vaddr + phdr.memsz) {
            m_register_state.rip = region.base() + header.entry - (phdr.vaddr & ~4095u);
        }

        size_t copy_offset = phdr.vaddr & 0xfffu;
        if (copy_offset + phdr.memsz > region.range().size()) {
            return Error::NoExec;
        }
        if (executable->read({reinterpret_cast<void *>(region.base() + copy_offset), phdr.filesz}, phdr.offset) !=
            phdr.filesz) {
            return Error::NoExec;
        }
    }

    // Setup user stack.
    ustd::Vector<uintptr_t> argv;
    auto push_arg = [&](ustd::StringView arg) {
        m_register_state.rsp -= ustd::align_up(arg.length() + 1, sizeof(size_t));
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

    m_register_state.rsp -= sizeof(uintptr_t);
    *reinterpret_cast<uintptr_t *>(m_register_state.rsp) = 0;
    for (uint32_t i = 0; i < argv.size(); i++) {
        m_register_state.rsp -= sizeof(uintptr_t);
        *reinterpret_cast<uintptr_t *>(m_register_state.rsp) = argv[argv.size() - i - 1];
    }

    m_register_state.rdi = argv.size();          // argc
    m_register_state.rsi = m_register_state.rsp; // argv

    // Allocate some space for heap storage.
    auto heap_object = VmObject::create(5_MiB);
    auto &heap_region = TRY(m_process->address_space().allocate_specific(
        {6_TiB, heap_object->size()}, RegionAccess::Writable | RegionAccess::UserAccessible));
    heap_region.map(ustd::move(heap_object));
    return {};
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
