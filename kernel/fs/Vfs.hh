#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/InodeId.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

namespace kernel {

class File;
class FileSystem;

struct Vfs {
    static void initialise();
    static void mount_root(ustd::UniquePtr<FileSystem> &&fs);

    static SysResult<InodeId> create(ustd::StringView path, const InodeId &base, InodeType type);
    static SysResult<> mkdir(ustd::StringView path, const InodeId &base = {});
    static SysResult<> mount(ustd::StringView path, FileSystem *fs);
    static SysResult<ustd::SharedPtr<File>> open(ustd::StringView path, OpenMode mode, const InodeId &base);
    static SysResult<InodeId> open_directory(ustd::StringView path, const InodeId &base);
    static InodeId root_inode();
};

} // namespace kernel
