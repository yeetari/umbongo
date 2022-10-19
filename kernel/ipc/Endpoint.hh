#pragma once

#include <kernel/Handle.hh>
#include <kernel/HandleKind.hh>
#include <kernel/SpinLock.hh>
#include <kernel/UserPtr.hh>
#include <ustd/Atomic.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace kernel {

class Channel;
class Region;
class Thread;
class VmObject;

class Endpoint : public ustd::Shareable<Endpoint> {
    USTD_ALLOW_MAKE_SHARED;
    Thread *const m_owner_thread;
    ustd::SharedPtr<Region> m_message_region;
    ustd::Vector<ustd::SharedPtr<Channel>> m_send_queue;
    bool m_owner_blocked{false};
    mutable SpinLock m_mutex;

    Endpoint(Thread *owner_thread, ustd::SharedPtr<Region> &&message_region);

public:
    static constexpr auto k_handle_kind = HandleKind::Endpoint;
    Endpoint(const Endpoint &) = delete;
    Endpoint(Endpoint &&) = delete;
    ~Endpoint();

    Endpoint &operator=(const Endpoint &) = delete;
    Endpoint &operator=(Endpoint &&) = delete;

    bool queue_sender(ustd::SharedPtr<Channel> channel);
    void set_owner_blocked(bool blocked) { m_owner_blocked = blocked; }

    Thread *owner_thread() const { return m_owner_thread; }
    Region &message_region() const { return *m_message_region; }
    ustd::Vector<ustd::SharedPtr<Channel>> &send_queue() { return m_send_queue; }
    SpinLock &mutex() { return m_mutex; }
    bool has_message_region() const { return !!m_message_region; }
};

} // namespace kernel
