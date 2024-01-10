#include <kernel/proc/process.hh>

#include <kernel/fs/file_handle.hh>
#include <kernel/fs/vfs.hh>
#include <kernel/mem/address_space.hh>
#include <kernel/mem/memory_manager.hh>
#include <kernel/mem/vm_object.hh>
#include <kernel/proc/thread.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {
namespace {

size_t s_pid_counter = 0;

} // namespace

Process::Process(bool is_kernel) : m_pid(s_pid_counter++), m_is_kernel(is_kernel), m_cwd(Vfs::root_inode()) {
    m_address_space = ustd::make_unique<AddressSpace>(*this);

    // TODO: Mapping this into every process is wasteful.
    constexpr auto access = RegionAccess::Writable | RegionAccess::Executable | RegionAccess::Global;
    auto *kernel_object = MemoryManager::kernel_object();
    auto &kernel_region = ASSUME(m_address_space->allocate_specific({0, kernel_object->size()}, access));
    kernel_region.map(ustd::adopt_shared(kernel_object));
}

Process::~Process() {
    ASSERT(m_thread_count.load() == 0);
}

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
