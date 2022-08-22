#include <kernel/fs/Vfs.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace kernel {
namespace {

class Mount {
    InodeId m_host;
    FileSystem *m_fs;

public:
    Mount(const InodeId &host, FileSystem *fs) : m_host(host), m_fs(ustd::move(fs)) {}

    const InodeId &host() const { return m_host; }
    FileSystem &fs() const { return *m_fs; }
};

// clang-format off
struct VfsData {
    InodeId root_inode;
    ustd::Vector<Mount> mounts;
} *s_data = nullptr;
// clang-format on

ustd::Optional<Mount &> find_mount(const InodeId &host) {
    for (auto &mount : s_data->mounts) {
        if (mount.host() == host) {
            return mount;
        }
    }
    return {};
}

InodeId resolve_path(ustd::StringView path, const InodeId &base, InodeId *out_parent = nullptr,
                     ustd::StringView *out_part = nullptr) {
    usize path_position = 0;
    auto consume_part = [&]() {
        if (path_position >= path.length()) {
            return ustd::StringView();
        }
        if (path[path_position] == '/') {
            path_position++;
            return ustd::StringView("/");
        }
        usize part_start = path_position;
        for (usize i = part_start; i < path.length(); i++) {
            if (path[i] == '/') {
                break;
            }
            path_position++;
        }
        return ustd::StringView(path.begin() + part_start, path_position - part_start);
    };

    InodeId current = path[0] == '/' ? s_data->root_inode : base;
    for (auto part = consume_part(); !part.empty(); part = consume_part()) {
        if (part[0] == '/') {
            continue;
        }
        if (out_parent != nullptr) {
            *out_parent = current;
        }
        if (current.is_null()) {
            return {};
        }
        current = current->lookup(part);
        if (out_part != nullptr) {
            *out_part = part;
        }
        if (current.is_null()) {
            auto remaining_part = consume_part();
            if (!remaining_part.empty() && remaining_part[0] != '/') {
                if (out_parent != nullptr) {
                    *out_parent = {};
                }
            }
        } else if (auto mount = find_mount(current)) {
            current = mount->fs().root_inode();
        }
    }
    return current;
}

} // namespace

void Vfs::initialise() {
    ASSERT(s_data == nullptr);
    s_data = new VfsData;
}

void Vfs::mount_root(ustd::UniquePtr<FileSystem> &&fs) {
    fs->mount(fs->root_inode(), {});
    s_data->root_inode = fs->root_inode();
    s_data->mounts.emplace(InodeId{}, ustd::move(fs).disown());
}

SysResult<InodeId> Vfs::create(ustd::StringView path, const InodeId &base, InodeType type) {
    InodeId parent;
    ustd::StringView name;
    if (!resolve_path(path, base, &parent, &name).is_null()) {
        return SysError::AlreadyExists;
    }
    if (parent.is_null()) {
        return SysError::NonExistent;
    }
    if (name.empty()) {
        return SysError::Invalid;
    }
    return TRY(parent->create(name, type));
}

SysResult<> Vfs::mkdir(ustd::StringView path, const InodeId &base) {
    TRY(create(path, base, InodeType::Directory));
    return {};
}

SysResult<> Vfs::mount(ustd::StringView path, FileSystem *fs) {
    InodeId parent;
    auto host = resolve_path(path, {}, &parent);
    if (host.is_null()) {
        return SysError::NonExistent;
    }
    fs->mount(parent, host);
    s_data->mounts.emplace(host, ustd::move(fs));
    return {};
}

SysResult<ustd::SharedPtr<File>> Vfs::open(ustd::StringView path, OpenMode mode, const InodeId &base) {
    auto inode = resolve_path(path, base);
    if (inode.is_null()) {
        if ((mode & OpenMode::Create) == OpenMode::Create) {
            return TRY(Vfs::create(path, base, InodeType::RegularFile))->open();
        }
        return SysError::NonExistent;
    }
    if ((mode & OpenMode::Truncate) == OpenMode::Truncate) {
        TRY(inode->truncate());
    }
    return TRY(inode->open());
}

SysResult<InodeId> Vfs::open_directory(ustd::StringView path, const InodeId &base) {
    auto inode = resolve_path(path, base);
    if (inode.is_null()) {
        return SysError::NonExistent;
    }
    if (inode->type() != InodeType::Directory) {
        return SysError::NotDirectory;
    }
    return inode;
}

InodeId Vfs::root_inode() {
    return s_data != nullptr ? s_data->root_inode : InodeId{};
}

} // namespace kernel
