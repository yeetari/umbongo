#include <kernel/fs/Vfs.hh>

#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/File.hh> // IWYU pragma: keep
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/Assert.hh>
#include <ustd/Result.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

class Mount {
    Inode *m_host;
    ustd::UniquePtr<FileSystem> m_fs;

public:
    Mount(Inode *host, ustd::UniquePtr<FileSystem> &&fs) : m_host(host), m_fs(ustd::move(fs)) {}

    Inode *host() const { return m_host; }
    FileSystem &fs() const { return *m_fs; }
};

// clang-format off
struct VfsData {
    Inode *root_inode{nullptr};
    ustd::Vector<Mount> mounts;
} *s_data = nullptr;
// clang-format on

Mount *find_mount(Inode *host) {
    for (auto &mount : s_data->mounts) {
        if (mount.host() == host) {
            return &mount;
        }
    }
    return nullptr;
}

Inode *resolve_path(ustd::StringView path, Inode *base, Inode **out_parent = nullptr,
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

    Inode *current = path[0] == '/' ? s_data->root_inode : base;
    for (auto part = consume_part(); !part.empty(); part = consume_part()) {
        if (part[0] == '/') {
            continue;
        }
        if (out_parent != nullptr) {
            *out_parent = current;
        }
        if (current == nullptr) {
            return nullptr;
        }
        current = current->lookup(part);
        if (out_part != nullptr) {
            *out_part = part;
        }
        if (current == nullptr) {
            auto remaining_part = consume_part();
            if (!remaining_part.empty() && remaining_part[0] != '/') {
                if (out_parent != nullptr) {
                    *out_parent = nullptr;
                }
            }
        } else if (auto *mount = find_mount(current)) {
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
    ASSERT(s_data->root_inode == nullptr);
    fs->mount(fs->root_inode(), nullptr);
    s_data->root_inode = fs->root_inode();
    s_data->mounts.emplace(nullptr, ustd::move(fs));
}

SysResult<Inode *> Vfs::create(ustd::StringView path, Inode *base, InodeType type) {
    Inode *parent = nullptr;
    ustd::StringView name;
    if (resolve_path(path, base, &parent, &name) != nullptr) {
        return SysError::AlreadyExists;
    }
    if (parent == nullptr) {
        return SysError::NonExistent;
    }
    if (name.empty()) {
        return SysError::Invalid;
    }
    return parent->create(name, type);
}

SysResult<> Vfs::mkdir(ustd::StringView path, Inode *base) {
    TRY(create(path, base, InodeType::Directory));
    return {};
}

SysResult<> Vfs::mount(ustd::StringView path, ustd::UniquePtr<FileSystem> &&fs) {
    Inode *parent = nullptr;
    auto *host = resolve_path(path, nullptr, &parent);
    if (host == nullptr) {
        return SysError::NonExistent;
    }
    fs->mount(parent, host);
    s_data->mounts.emplace(host, ustd::move(fs));
    return {};
}

SysResult<ustd::SharedPtr<File>> Vfs::open(ustd::StringView path, OpenMode mode, Inode *base) {
    auto *inode = resolve_path(path, base);
    if (inode == nullptr) {
        if ((mode & OpenMode::Create) == OpenMode::Create) {
            return TRY(Vfs::create(path, base, InodeType::RegularFile))->open();
        }
        return SysError::NonExistent;
    }
    if ((mode & OpenMode::Truncate) == OpenMode::Truncate) {
        inode->truncate();
    }
    return TRY(inode->open());
}

SysResult<Inode *> Vfs::open_directory(ustd::StringView path, Inode *base) {
    auto *inode = resolve_path(path, base);
    if (inode == nullptr) {
        return SysError::NonExistent;
    }
    if (inode->type() != InodeType::Directory) {
        return SysError::NotDirectory;
    }
    return inode;
}

Inode *Vfs::root_inode() {
    if (s_data == nullptr) {
        return nullptr;
    }
    return s_data->root_inode;
}
