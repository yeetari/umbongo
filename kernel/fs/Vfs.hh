#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/StringView.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

class File;
class FileSystem;
class Inode;

struct Vfs {
    static void initialise();
    static void mount_root(UniquePtr<FileSystem> &&fs);

    static SharedPtr<File> create(StringView path, Inode *base);
    static void mkdir(StringView path, Inode *base);
    static void mount(StringView path, UniquePtr<FileSystem> &&fs);
    static SharedPtr<File> open(StringView path, OpenMode mode, Inode *base);
    static Inode *open_directory(StringView path, Inode *base);
    static Inode *root_inode();
};
