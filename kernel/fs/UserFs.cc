#include <kernel/fs/UserFs.hh>

#include <kernel/ScopedLock.hh>
#include <kernel/api/UserFs.hh>
#include <kernel/fs/InodeFile.hh>
#include <kernel/proc/Scheduler.hh>

namespace kernel {

UserFs &UserFsInode::fs() const {
    return static_cast<UserFs &>(id().fs());
}

SysResult<InodeId> UserFsInode::child(usize index) const {
    fs().send_request({
        .index = index,
        .opcode = UserFsOpcode::Child,
    });
    ustd::LargeVector<char> read_data;
    auto response = fs().wait_response(read_data);
    if (response.error != static_cast<SysError>(0)) {
        return response.error;
    }
    return InodeId(fs(), response.inode_index);
}

SysResult<InodeId> UserFsInode::create(ustd::StringView, InodeType) {
    ENSURE_NOT_REACHED();
}

InodeId UserFsInode::lookup(ustd::StringView name) {
    fs().send_request(
        {
            .data_size = name.length(),
            .opcode = UserFsOpcode::Read,
        },
        name);
    ustd::LargeVector<char> read_data;
    auto response = fs().wait_response(read_data);
    if (response.error != static_cast<SysError>(0)) {
        return {};
    }
    return {fs(), response.inode_index};
}

usize UserFsInode::read(ustd::Span<void> data, usize offset) const {
    fs().send_request({
        .offset = offset,
        .opcode = UserFsOpcode::Read,
    });
    ustd::LargeVector<char> read_data;
    auto response = fs().wait_response(read_data);
    usize size = ustd::min(data.size(), response.data_size);
    __builtin_memcpy(data.data(), read_data.data(), size);
    return size;
}

SysResult<> UserFsInode::remove(ustd::StringView) {
    ENSURE_NOT_REACHED();
}

usize UserFsInode::size() const {
    fs().send_request({
        .opcode = UserFsOpcode::Size,
    });
    ustd::LargeVector<char> read_data;
    auto response = fs().wait_response(read_data);
    if (response.error != static_cast<SysError>(0)) {
        return 0;
    }
    return response.size;
}

SysResult<> UserFsInode::truncate() {
    ENSURE_NOT_REACHED();
}

usize UserFsInode::write(ustd::Span<const void>, usize) {
    ENSURE_NOT_REACHED();
}

ustd::StringView UserFsInode::name() const {
    return "foo";
}

void UserFs::send_request(const UserFsRequest &request, ustd::StringView view) {
    ScopedLock locker(m_lock);
    const auto request_size = sizeof(UserFsRequest) + request.data_size;
    m_request_queue.ensure_size(m_request_queue.size() + request_size);
    __builtin_memcpy(&m_request_queue[m_request_queue.size() - request_size], &request, request_size);
    ASSERT(view.length() == request.data_size);
    if (view.length() != 0) {
        __builtin_memcpy(&m_request_queue[m_request_queue.size() - request_size + sizeof(UserFsRequest)], view.data(),
                         view.length());
    }
}

UserFsResponse UserFs::wait_response(ustd::LargeVector<char> &data) {
    ScopedLock locker(m_lock);
    while (m_pending_response.empty()) {
        m_lock.unlock();
        Scheduler::yield(true);
        m_lock.lock();
    }
    UserFsResponse response{};
    __builtin_memcpy(&response, m_pending_response.data(), sizeof(UserFsResponse));
    data.ensure_size(response.data_size);
    __builtin_memcpy(data.data(), m_pending_response.data() + sizeof(UserFsResponse), response.data_size);
    m_pending_response.clear();
    return response;
}

bool UserFs::read_would_block(usize) const {
    ScopedLock locker(m_lock);
    return m_request_queue.empty();
}

bool UserFs::write_would_block(usize) const {
    ScopedLock locker(m_lock);
    return !m_pending_response.empty();
}

SysResult<usize> UserFs::read(ustd::Span<void> data, usize) {
    ScopedLock locker(m_lock);
    usize bytes_read = 0;
    while (!m_request_queue.empty()) {
        auto *request = &m_request_queue[0];
        auto request_size = sizeof(UserFsRequest) + reinterpret_cast<UserFsRequest *>(request)->data_size;
        if (data.size() < request_size || data.size() - bytes_read < request_size) {
            break;
        }
        __builtin_memcpy(&data.as<char>()[bytes_read], request, request_size);
        bytes_read += request_size;
        for (usize i = 0; i < request_size; i++) {
            m_request_queue.take(0);
        }
    }
    return bytes_read;
}

SysResult<usize> UserFs::write(ustd::Span<const void> data, usize) {
    ScopedLock locker(m_lock);
    ENSURE(m_pending_response.empty());
    ENSURE(data.size() >= sizeof(UserFsResponse));
    const auto *response = reinterpret_cast<const UserFsResponse *>(data.data());
    m_pending_response.ensure_size(sizeof(UserFsResponse) + response->data_size);
    ENSURE(data.size() == m_pending_response.size());
    __builtin_memcpy(m_pending_response.data(), response, m_pending_response.size());
    return data.size();
}

void UserFs::mount(const InodeId &parent, const InodeId &) {
    ASSERT(!m_root_inode);
    auto &root = m_root_inode.emplace(root_inode(), parent, InodeType::Directory);
    root.bind_anonymous_file(ustd::make_shared<InodeFile>(root_inode()));
}

Inode *UserFs::inode(const InodeId &id) {
    ASSERT(id.index() == 0);
    ASSERT(m_root_inode);
    return m_root_inode.obj();
}

} // namespace kernel
