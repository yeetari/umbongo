#pragma once

#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/fs/InodeType.hh>
#include <ustd/SharedPtr.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

class File;
class FileSystem;
class Inode;

struct Vfs {
    static void initialise();
    static void mount_root(UniquePtr<FileSystem> &&fs);

    static SysResult<Inode *> create(StringView path, Inode *base, InodeType type);
    static SysResult<> mkdir(StringView path, Inode *base);
    static SysResult<> mount(StringView path, UniquePtr<FileSystem> &&fs);
    static SysResult<SharedPtr<File>> open(StringView path, OpenMode mode, Inode *base);
    static SysResult<Inode *> open_directory(StringView path, Inode *base);
    static Inode *root_inode();
};
