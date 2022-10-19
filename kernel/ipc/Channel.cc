#include <kernel/ipc/Channel.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/api/Constants.h>
#include <kernel/ipc/Endpoint.hh>
#include <kernel/mem/VmObject.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/SharedPtr.hh>

namespace kernel {

Channel::Channel(ustd::SharedPtr<Endpoint> &&endpoint, Process &owner_process, ustd::SharedPtr<VmObject> &&message_vmo)
    : m_endpoint(ustd::move(endpoint)), m_owner_process(owner_process), m_message_vmo(ustd::move(message_vmo)) {
    if (m_message_vmo) {
        m_message_region = EXPECT(owner_process.virt_space().allocate_anywhere(RegionAccess::Writable, m_message_vmo));
    }
}

Channel::~Channel() = default;

uint8_t *Channel::copy_base() const {
    return reinterpret_cast<uint8_t *>(m_message_region->base());
}

size_t Channel::max_size() const {
    return m_message_region->range().size();
}

void Channel::set_transfer_handles(ustd::Vector<ustd::UniquePtr<HandleBase>> &&transfer_handles) {
    m_transfer_handles = ustd::move(transfer_handles);
}

void Channel::add_pending_thread(Thread *thread) {
    ScopedLock lock(m_pending_threads_mutex);
    m_pending_threads.push(thread);
}

void Channel::wake_one_pending() {
    ScopedLock lock(m_pending_threads_mutex);
    if (!m_pending_threads.empty()) {
        m_pending_threads.take(0)->unblock();
    }
}

SyscallResult Thread::sys_chnl_create(ub_handle_t endp_handle, ub_handle_t vmo_handle) {
    ustd::SharedPtr<VmObject> message_vmo;
    if (vmo_handle != UB_NULL_HANDLE) {
        message_vmo = TRY(m_process->lookup_handle<VmObject>(vmo_handle));
    }

    auto endpoint = TRY(m_process->lookup_handle<Endpoint>(endp_handle));
    auto channel = ustd::make_shared<Channel>(ustd::move(endpoint), *m_process, ustd::move(message_vmo));
    return m_process->allocate_handle(ustd::move(channel));
}

SyscallResult Thread::sys_chnl_send(ub_handle_t chnl_handle, UserPtr<uint8_t> user_data, size_t data_size,
                                    UserPtr<ub_handle_t> user_transfer_handles, uint32_t transfer_handle_count) {
    ustd::Array<ub_handle_t, UB_MAX_HANDLE_COUNT> transfer_handles{};
    if (transfer_handle_count > transfer_handles.size()) {
        return Error::TooBig;
    }

    ustd::Vector<ustd::UniquePtr<HandleBase>> copied_handles;
    if (transfer_handle_count != 0) {
        TRY(copy_from_user(transfer_handles.data(), user_transfer_handles, transfer_handle_count));
        copied_handles.ensure_capacity(transfer_handle_count);
        for (uint32_t i = 0; i < transfer_handle_count; i++) {
            copied_handles.push(TRY(m_process->lookup_handle(transfer_handles[i])).copy());
        }
    }

    auto channel = TRY(m_process->lookup_handle<Channel>(chnl_handle));
    if (data_size != 0 && !channel->has_message_region()) {
        // TODO: Better error.
        return Error::BadAccess;
    }
    if (data_size > channel->max_size()) {
        return Error::TooBig;
    }

    auto &endpoint = channel->endpoint();
    if (!ustd::atomic_cmpxchg(channel->send_pending(), false, true)) {
        // Either another thread is waiting to be serviced by the receiver, or (more unlikely), a failed race with
        // another thread.
        channel->add_pending_thread(this);
        set_state(ThreadState::Blocked);
        Scheduler::yield_to(endpoint.owner_thread());

        // Should be a success as only one pending thread (ourselves) should have been woken.
        [[maybe_unused]] bool success = ustd::atomic_cmpxchg(channel->send_pending(), false, true);
        ASSERT(success);
    }

    ScopedLock lock(channel->mutex());
    endpoint.queue_sender(channel);

    if (data_size != 0) {
        TRY(copy_from_user(channel->copy_base(), user_data, data_size));
    }
    channel->set_transfer_handles(ustd::move(copied_handles));
    return 0;
}

} // namespace kernel
