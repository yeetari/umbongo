#include <kernel/fs/Vfs.hh>

#include <kernel/fs/File.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <ustd/Assert.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

class Mount {
    UniquePtr<FileSystem> m_fs;

public:
    explicit Mount(UniquePtr<FileSystem> &&fs) : m_fs(ustd::move(fs)) {}
};

// clang-format off
struct VfsData {
    Inode *root_inode{nullptr};
    Vector<Mount> mounts;
} *s_data;
// clang-format on

Inode *resolve_path(StringView path, Inode **out_parent = nullptr, StringView *out_part = nullptr) {
    usize path_position = 0;
    auto consume_part = [&]() {
        if (path_position >= path.length()) {
            return StringView();
        }
        if (path[path_position] == '/') {
            path_position++;
            return StringView("/");
        }
        usize part_start = path_position;
        for (usize i = part_start; i < path.length(); i++) {
            if (path[i] == '/') {
                break;
            }
            path_position++;
        }
        return StringView(path.begin() + part_start, path_position - part_start);
    };

    Inode *current = nullptr;
    for (auto part = consume_part(); !part.empty(); part = consume_part()) {
        if (part[0] == '/') {
            if (current == nullptr) {
                // Absolute path.
                current = s_data->root_inode;
            }
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
        }
    }
    return current;
}

} // namespace

void Vfs::initialise() {
    s_data = new VfsData;
}

void Vfs::mount_root(UniquePtr<FileSystem> &&fs) {
    ASSERT(s_data->root_inode == nullptr);
    s_data->root_inode = fs->root_inode();
    s_data->mounts.emplace(ustd::move(fs));
}

SharedPtr<File> Vfs::create(StringView path) {
    Inode *parent = nullptr;
    StringView name;
    if (resolve_path(path, &parent, &name) != nullptr) {
        ENSURE_NOT_REACHED("create on existing path");
    }
    ASSERT(parent != nullptr && !name.empty());
    auto *inode = parent->create(name, InodeType::RegularFile);
    return inode->open();
}

void Vfs::mkdir(StringView path) {
    Inode *parent = nullptr;
    StringView name;
    if (resolve_path(path, &parent, &name) != nullptr) {
        ENSURE_NOT_REACHED("mkdir on existing path");
    }
    ASSERT(parent != nullptr && !name.empty());
    parent->create(name, InodeType::Directory);
}

SharedPtr<File> Vfs::open(StringView path) {
    auto *inode = resolve_path(path);
    if (inode == nullptr) {
        return {};
    }
    return inode->open();
}
