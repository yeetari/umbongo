#pragma once

#include <kernel/api/types.h>
#include <kernel/sys_result.hh>
#include <ustd/shared_ptr.hh> // IWYU pragma: keep
#include <ustd/string_view.hh>
#include <ustd/unique_ptr.hh> // IWYU pragma: keep

namespace kernel {

class File;
class FileSystem;
class Inode;
enum class InodeType;

struct Vfs {
    static void initialise();
    static void mount_root(ustd::UniquePtr<FileSystem> &&fs);

    static SysResult<Inode *> create(ustd::StringView path, Inode *base, InodeType type);
    static SysResult<> mkdir(ustd::StringView path, Inode *base);
    static SysResult<> mount(ustd::StringView path, ustd::UniquePtr<FileSystem> &&fs);
    static SysResult<ustd::SharedPtr<File>> open(ustd::StringView path, ub_open_mode_t mode, Inode *base);
    static SysResult<Inode *> open_directory(ustd::StringView path, Inode *base);
    static Inode *root_inode();
};

} // namespace kernel
