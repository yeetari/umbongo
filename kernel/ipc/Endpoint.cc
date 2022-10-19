#include <kernel/ipc/Endpoint.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/SpinLock.hh>
#include <kernel/api/Constants.h>
#include <kernel/api/Types.h>
#include <kernel/ipc/Channel.hh>
#include <kernel/mem/VmObject.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <ustd/Optional.hh>
#include <ustd/ScopeGuard.hh>
#include <ustd/Try.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

Endpoint::Endpoint(Thread *owner_thread, ustd::SharedPtr<Region> &&message_region)
    : m_owner_thread(owner_thread), m_message_region(ustd::move(message_region)) {}

Endpoint::~Endpoint() = default;

bool Endpoint::queue_sender(ustd::SharedPtr<Channel> channel) {
    ScopedLock lock(m_mutex);
    m_send_queue.push(ustd::move(channel));
    if (ustd::exchange(m_owner_blocked, false)) {
        ASSERT(m_send_queue.size() == 1);
        m_owner_thread->unblock();
        return true;
    }
    return false;
}

SyscallResult Thread::sys_endp_create(ub_handle_t vmr_handle) {
    ustd::SharedPtr<Region> message_region;
    if (vmr_handle != UB_NULL_HANDLE) {
        message_region = TRY(m_process->lookup_handle<Region>(vmr_handle));
    }
    auto endpoint = ustd::make_shared<Endpoint>(this, ustd::move(message_region));
    return m_process->allocate_handle(ustd::move(endpoint));
}

SyscallResult Thread::sys_endp_recv(ub_handle_t endp_handle, UserPtr<ub_handle_t> user_handles) {
    auto endpoint = TRY(m_process->lookup_handle<Endpoint>(endp_handle));
    if (endpoint->owner_thread() != this) {
        return Error::BadAccess;
    }

    if (ScopedLock lock(endpoint->mutex()); endpoint->send_queue().empty()) {
        // No sender in queue, block until something is sent.
        endpoint->set_owner_blocked(true);
        lock.unlock();
        set_state(ThreadState::Blocked);
        Scheduler::yield(true);
    }

    ScopedLock endpoint_lock(endpoint->mutex());
    ASSERT(!endpoint->send_queue().empty());

    auto channel = endpoint->send_queue().take(0);
    ASSERT(&channel->endpoint() == endpoint.ptr());

    ustd::Vector<ustd::UniquePtr<HandleBase>> transfer_handles;
    {
        // Taking the lock also important for ensuring data has been copied client-side (from arbritary region such as
        // stack to the message VMO).
        ScopedLock channel_lock(channel->mutex());
        transfer_handles = ustd::move(channel->transfer_handles());
    }

    ASSERT(ustd::atomic_load(channel->send_pending()));
    ustd::atomic_store(channel->send_pending(), false);
    channel->wake_one_pending();

    // Map data buffer.
    if (endpoint->has_message_region()) {
        if (auto message_vmo = channel->message_vmo()) {
            endpoint->message_region().map(ustd::move(message_vmo));
        }
    }

    // Transfer handles across.
    ustd::Array<ub_handle_t, UB_MAX_HANDLE_COUNT> user_transfer_handles{};
    for (size_t i = 0; auto &handle : transfer_handles) {
        user_transfer_handles[i++] = m_process->allocate_handle(ustd::move(handle));
    }
    TRY(copy_to_user(user_handles, user_transfer_handles.data(), transfer_handles.size()));
    return transfer_handles.size();
}

} // namespace kernel
