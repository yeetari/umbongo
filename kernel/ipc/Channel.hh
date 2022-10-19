#pragma once

#include <kernel/HandleKind.hh>
#include <kernel/SpinLock.hh>
#include <ustd/Optional.hh>
#include <ustd/Shareable.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace kernel {

class Endpoint;
class HandleBase;
class Process;
class Region;
class Thread;
class VmObject;

class Channel : public ustd::Shareable<Channel> {
    USTD_ALLOW_MAKE_SHARED;
    const ustd::SharedPtr<Endpoint> m_endpoint;
    Process &m_owner_process;
    ustd::SharedPtr<VmObject> m_message_vmo;
    ustd::SharedPtr<Region> m_message_region;
    ustd::Vector<ustd::UniquePtr<HandleBase>> m_transfer_handles;
    ustd::Vector<Thread *> m_pending_threads;
    bool m_send_pending{false};
    mutable SpinLock m_mutex;
    mutable SpinLock m_pending_threads_mutex;

    Channel(ustd::SharedPtr<Endpoint> &&endpoint, Process &owner_process, ustd::SharedPtr<VmObject> &&message_vmo);

public:
    static constexpr auto k_handle_kind = HandleKind::Channel;
    Channel(const Channel &) = delete;
    Channel(Channel &&) = delete;
    ~Channel();

    Channel &operator=(const Channel &) = delete;
    Channel &operator=(Channel &&) = delete;

    uint8_t *copy_base() const;
    size_t max_size() const;
    void set_transfer_handles(ustd::Vector<ustd::UniquePtr<HandleBase>> &&transfer_handles);

    void add_pending_thread(Thread *thread);
    void wake_one_pending();

    Endpoint &endpoint() const { return *m_endpoint; }
    const ustd::SharedPtr<VmObject> &message_vmo() const { return m_message_vmo; }
    ustd::Vector<ustd::UniquePtr<HandleBase>> &transfer_handles() { return m_transfer_handles; }
    bool &send_pending() { return m_send_pending; }
    SpinLock &mutex() const { return m_mutex; }
    bool has_message_region() const { return !!m_message_region; }
};

} // namespace kernel
