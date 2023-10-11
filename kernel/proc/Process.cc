#include <kernel/proc/Process.hh>

#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/proc/Thread.hh>
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

class VirtSpace;

Process::Process(bool is_kernel, ustd::SharedPtr<VirtSpace> virt_space)
    : m_pid(s_pid_counter++), m_is_kernel(is_kernel), m_cwd(Vfs::root_inode()), m_virt_space(ustd::move(virt_space)) {}

uint32_t Process::allocate_fd() {
    // TODO: Limits/error handling. Allocating 2^32 fds would overflow the vector.
    for (uint32_t i = 0; i < m_fds.size(); i++) {
        if (!m_fds[i]) {
            return i;
        }
    }
    m_fds.emplace();
    return m_fds.size() - 1;
}

ustd::UniquePtr<Thread> Process::create_thread(ThreadPriority priority) {
    return ustd::make_unique<Thread>(this, priority);
}

} // namespace kernel
