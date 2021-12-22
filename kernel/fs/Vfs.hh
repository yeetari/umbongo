#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

namespace kernel {

class File;
class FileSystem;
class Inode;

struct Vfs {
    static void initialise();
    static void mount_root(ustd::UniquePtr<FileSystem> &&fs);

    static SysResult<Inode *> create(ustd::StringView path, Inode *base, InodeType type);
    static SysResult<> mkdir(ustd::StringView path, Inode *base);
    static SysResult<> mount(ustd::StringView path, ustd::UniquePtr<FileSystem> &&fs);
    static SysResult<ustd::SharedPtr<File>> open(ustd::StringView path, OpenMode mode, Inode *base);
    static SysResult<Inode *> open_directory(ustd::StringView path, Inode *base);
    static Inode *root_inode();
};

} // namespace kernel
