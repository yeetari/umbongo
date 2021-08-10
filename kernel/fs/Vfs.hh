#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/StringView.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

class File;
class FileSystem;

struct Vfs {
    static void initialise();
    static void mount_root(UniquePtr<FileSystem> &&fs);

    static SharedPtr<File> create(StringView path);
    static void mkdir(StringView path);
    static void mount(StringView path, UniquePtr<FileSystem> &&fs);
    static SharedPtr<File> open(StringView path, OpenMode mode);
};
