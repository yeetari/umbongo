#pragma once

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
    static SharedPtr<File> open(StringView path);
};