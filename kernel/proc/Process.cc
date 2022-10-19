#include <kernel/proc/Process.hh>

#include <boot/BootInfo.hh>
#include <kernel/ScopedLock.hh>
#include <kernel/mem/VirtSpace.hh> // IWYU pragma: keep
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {
namespace {

size_t s_pid_counter = 0;

} // namespace

Process::Process(bool is_kernel, ustd::String &&name, ustd::SharedPtr<VirtSpace> virt_space)
    : m_pid(s_pid_counter++), m_is_kernel(is_kernel), m_name(ustd::move(name)), m_virt_space(ustd::move(virt_space)) {}

ub_handle_t Process::allocate_handle(ustd::UniquePtr<HandleBase> &&handle) {
    // TODO: Handle reuse + handle limit.
    ScopedLock lock(m_handle_table_lock);
    for (ub_handle_t index = 0; index < m_handle_table.size(); index++) {
        if (!m_handle_table[index]) {
            m_handle_table[index] = ustd::move(handle);
            return index;
        }
    }
    m_handle_table.push(ustd::move(handle));
    return m_handle_table.size() - 1;
}

ustd::UniquePtr<Thread> Process::create_thread(ThreadPriority priority) {
    return ustd::make_unique<Thread>(this, priority);
}

SysResult<> Process::close_handle(ub_handle_t index) {
    ScopedLock lock(m_handle_table_lock);
    if (index >= m_handle_table.size() || !m_handle_table[index]) {
        return Error::BadHandle;
    }
    m_handle_table[index].clear();
    return {};
}

SysResult<HandleBase &> Process::lookup_handle(ub_handle_t index) {
    ScopedLock lock(m_handle_table_lock);
    if (index >= m_handle_table.size() || !m_handle_table[index]) {
        return Error::BadHandle;
    }
    return *m_handle_table[index];
}

SyscallResult Thread::sys_handle_close(ub_handle_t handle) {
    TRY(m_process->close_handle(handle));
    return 0;
}

SyscallResult Thread::sys_proc_create(UserPtr<const uint8_t> binary, size_t binary_size, UserPtr<const char> name,
                                      size_t name_length) {
    ustd::String name_copy;
    name_length = ustd::min(name_length, 128ul);
    if (name_length != 0) {
        name_copy = ustd::String(name_length);
        TRY(copy_from_user(name_copy.data(), name, name_length));
    }

    auto new_thread = Thread::create_user(ThreadPriority::Normal, ustd::move(name_copy));
    TRY(new_thread->exec(binary, binary_size));

    auto &new_process = new_thread->process();
    for (ScopedLock lock(m_process->m_handle_table_lock); const auto &handle : m_process->m_handle_table) {
        if (handle && handle->kind() == HandleKind::Endpoint) {
            new_process.m_handle_table.push(handle->copy());
        }
    }

    Scheduler::insert_thread(ustd::move(new_thread));
    return new_process.pid();
}

SyscallResult Thread::sys_proc_exit(size_t) {
    set_state(ThreadState::Dead);
    Scheduler::yield(false);
    ENSURE_NOT_REACHED();
}

} // namespace kernel
