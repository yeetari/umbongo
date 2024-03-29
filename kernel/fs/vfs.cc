#include <kernel/fs/vfs.hh>

#include <kernel/api/types.h>
#include <kernel/error.hh>
#include <kernel/fs/file.hh>
#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/optional.hh>
#include <ustd/shared_ptr.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace kernel {
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

ustd::Optional<Mount &> find_mount(Inode *host) {
    for (auto &mount : s_data->mounts) {
        if (mount.host() == host) {
            return mount;
        }
    }
    return {};
}

Inode *resolve_path(ustd::StringView path, Inode *base, Inode **out_parent = nullptr,
                    ustd::StringView *out_part = nullptr) {
    size_t path_position = 0;
    auto consume_part = [&]() {
        if (path_position >= path.length()) {
            return ustd::StringView();
        }
        if (path[path_position] == '/') {
            path_position++;
            return ustd::StringView("/");
        }
        size_t part_start = path_position;
        for (size_t i = part_start; i < path.length(); i++) {
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
    ASSERT(s_data->root_inode == nullptr);
    fs->mount(fs->root_inode(), nullptr);
    s_data->root_inode = fs->root_inode();
    s_data->mounts.emplace(nullptr, ustd::move(fs));
}

SysResult<Inode *> Vfs::create(ustd::StringView path, Inode *base, InodeType type) {
    Inode *parent = nullptr;
    ustd::StringView name;
    if (resolve_path(path, base, &parent, &name) != nullptr) {
        return Error::AlreadyExists;
    }
    if (parent == nullptr) {
        return Error::NonExistent;
    }
    if (name.empty()) {
        return Error::Invalid;
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
        return Error::NonExistent;
    }
    fs->mount(parent, host);
    s_data->mounts.emplace(host, ustd::move(fs));
    return {};
}

SysResult<ustd::SharedPtr<File>> Vfs::open(ustd::StringView path, ub_open_mode_t mode, Inode *base) {
    auto *inode = resolve_path(path, base);
    if (inode == nullptr) {
        if ((mode & UB_OPEN_MODE_CREATE) == UB_OPEN_MODE_CREATE) {
            return TRY(Vfs::create(path, base, InodeType::RegularFile))->open();
        }
        return Error::NonExistent;
    }
    if ((mode & UB_OPEN_MODE_TRUNCATE) == UB_OPEN_MODE_TRUNCATE) {
        TRY(inode->truncate());
    }
    return TRY(inode->open());
}

SysResult<Inode *> Vfs::open_directory(ustd::StringView path, Inode *base) {
    auto *inode = resolve_path(path, base);
    if (inode == nullptr) {
        return Error::NonExistent;
    }
    if (inode->type() != InodeType::Directory) {
        return Error::NotDirectory;
    }
    return inode;
}

Inode *Vfs::root_inode() {
    return s_data != nullptr ? s_data->root_inode : nullptr;
}

} // namespace kernel
