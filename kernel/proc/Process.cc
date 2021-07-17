#include <kernel/proc/Process.hh>

#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Assert.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

usize s_pid_counter = 0;

} // namespace

Process *Process::create_kernel() {
    return new Process(s_pid_counter++, true, MemoryManager::kernel_space());
}

Process *Process::create_user(VirtSpace *virt_space) {
    return new Process(s_pid_counter++, false, virt_space);
}

Process::Process(usize pid, bool is_kernel, VirtSpace *virt_space)
    : m_pid(pid), m_is_kernel(is_kernel), m_virt_space(virt_space) {
    m_register_state.cs = is_kernel ? 0x08 : (0x20u | 0x3u);
    m_register_state.ss = is_kernel ? 0x10 : (0x18u | 0x3u);
    m_register_state.rflags = 0x202;
    if (is_kernel) {
        m_register_state.rsp = reinterpret_cast<uintptr>(new char[4_KiB] + 4_KiB);
    } else {
        m_register_state.rsp = k_user_stack_head;
    }
}

Process::~Process() {
    // TODO: Delete virt space if not kernel task.
    ENSURE_NOT_REACHED();
}

uint32 Process::allocate_fd() {
    // TODO: Limits/error handling. Allocating 2^32 fds would overflow the vector.
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (!m_fds[i]) {
            return i;
        }
    }
    m_fds.emplace();
    return m_fds.size() - 1;
}

void Process::kill() {
    m_state = ProcessState::Dead;
}

void Process::set_entry_point(uintptr entry) {
    m_register_state.rip = m_is_kernel ? entry : k_user_binary_base + entry;
}
