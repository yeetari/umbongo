#include "Ufs.hh"

#include "Drive.hh"
#include "UfsTypes.hh"

#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Syscall.hh>
#include <kernel/api/UserFs.hh>
#include <log/Log.hh>
#include <ustd/Numeric.hh>
#include <ustd/Types.hh>

usize UfsInode::read_bytes(ustd::Span<void> data, usize offset) {
    ENSURE(offset == 0);
    if (m_header.size <= (m_fs.block_size() - sizeof(ufs::InodeHeader))) {
        // Inode contents is inline in the header.
        ustd::Span<void> span(data.data(), ustd::min(data.size(), m_header.size));
        return m_fs.drive().read(span, m_block_index, sizeof(ufs::InodeHeader));
    }
    ENSURE_NOT_REACHED();
}

void UfsInode::traverse_directory() {
    // NOLINTNEXTLINE
    ustd::Array<uint8, 4_KiB> buffer;
    for (usize offset = 0; offset < m_header.size; offset += buffer.size_bytes()) {
        usize bytes_read = read_bytes({buffer.data(), m_fs.block_size()}, offset);
        usize entry_count = bytes_read / sizeof(ufs::DirectoryEntry);
        for (usize i = 0; i < entry_count; i++) {
            auto &entry = buffer.span().as<ufs::DirectoryEntry>()[i];
            log::debug("Entry {}: {}", i, ustd::StringView(static_cast<const char *>(entry.name), entry.name_length));
            if (entry.inode != 0) {
                m_fs.read_inode(entry.inode)->traverse_directory();
            }
        }
    }
}

InodePtr<UfsInode> Ufs::read_inode(ufs::BlockIndex index) {
    const auto &group = m_group_headers[index / m_inodes_per_group];
    ufs::BlockIndex block_index = group.block_table + (index % m_inodes_per_group);
    auto header = m_drive.read<ufs::InodeHeader>(block_index);
    return new UfsInode(*this, block_index, header);
}

ustd::Result<void, core::SysError> Ufs::initialise(core::EventLoop &event_loop) {
    auto superblock = m_drive.read<ufs::Superblock>(0);
    ENSURE(superblock.magic == ufs::k_superblock_magic);
    m_drive.set_block_size(m_block_size = superblock.block_size);

    uint64 fs_size = superblock.block_count * superblock.block_size;
    uint64 max_fs_size = (static_cast<uint64>(ustd::Limits<uint32>::max()) + 1) * superblock.block_size;
    log::debug("Block size: {}", superblock.block_size);
    log::debug("FS size: {:s} (max: {:s})", fs_size, max_fs_size);

    uint32 block_group_count = ustd::ceil_div(superblock.block_count, ufs::blocks_per_group(superblock.block_size));
    m_group_headers.ensure_size(block_group_count);

    m_drive.read(m_group_headers.span(), 1);
    for (uint32 i = 0; const auto &group : m_group_headers) {
        log::trace("Group {}: block_bitmap={} block_table={}", i++, group.block_bitmap, group.block_table);
    }

    m_inodes_per_group = ufs::blocks_per_group(superblock.block_size) - 1;
    log::trace("inodes_per_group: {}", m_inodes_per_group);

    ENSURE((superblock.block_size - sizeof(ufs::InodeHeader)) % sizeof(ufs::BlockIndex) == 0);
    m_block_indices_per_inode = (superblock.block_size - sizeof(ufs::InodeHeader)) / sizeof(ufs::BlockIndex);
    log::trace("block_indices_per_inode: {}", m_block_indices_per_inode);

    m_user_fs = core::File(EXPECT(core::syscall<uint32>(Syscall::create_user_fs, "/ufs")));
    m_user_fs.set_on_read_ready([this] {
        EXPECT(m_user_fs.read<kernel::UserFsRequest>());
        EXPECT(m_user_fs.write(kernel::UserFsResponse{
            .size = 1,
        }));
    });
    event_loop.watch(m_user_fs, kernel::PollEvents::Read);

    m_root_inode = read_inode(0);
    ENSURE(m_root_inode->is_directory());
    m_root_inode->traverse_directory();
    return {};
}
