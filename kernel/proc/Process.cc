#include <kernel/proc/Process.hh>

#include <kernel/mem/VirtSpace.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace {

usize s_pid_counter = 0;

} // namespace

Process *Process::create_kernel() {
    return new Process(s_pid_counter++, true, {});
}

Process *Process::create_user(void *binary, usize binary_size) {
    return new Process(s_pid_counter++, false, VirtSpace::create_user(binary, binary_size));
}

Process::Process(usize pid, bool is_kernel, VirtSpace &&virt_space)
    : m_pid(pid), m_is_kernel(is_kernel), m_virt_space(ustd::move(virt_space)) {
    m_register_state.cs = is_kernel ? 0x08 : (0x20u | 0x3u);
    m_register_state.ss = is_kernel ? 0x10 : (0x18u | 0x3u);
    m_register_state.rflags = 0x202;
    if (is_kernel) {
        m_register_state.rsp = reinterpret_cast<uintptr>(new char[4_KiB] + 4_KiB);
    } else {
        m_register_state.rsp = k_user_stack_head;
    }
}

void Process::kill() {
    m_state = ProcessState::Dead;
}

void Process::set_entry_point(uintptr entry) {
    m_register_state.rip = m_is_kernel ? entry : k_user_binary_base + entry;
}
