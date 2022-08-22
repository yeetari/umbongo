#define USTD_NO_MEMORY
#include <servers/fs/UfsTypes.hh>
#include <ustd/Function.hh>
#include <ustd/Numeric.hh>
#include <ustd/Span.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <unistd.h>

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    std::fprintf(stderr, "Assertion '%s' failed at %s:%u\n", expr, file, line);
    if (msg != nullptr) {
        std::fprintf(stderr, "=> %s\n", msg);
    }
    std::abort();
}

namespace {

class BlockGroup;

class Inode {
    const BlockGroup &m_block_group;
    const ufs::BlockIndex m_block_index;
    ufs::InodeHeader *const m_header;

public:
    Inode(const BlockGroup &block_group, const ufs::BlockIndex block_index, ufs::InodeHeader *header)
        : m_block_group(block_group), m_block_index(block_index), m_header(header) {}

    void write(ustd::Span<const void> data) const;

    ufs::BlockIndex inode_index() const { return m_block_index - 1; }
};

class BlockGroup {
    const uint16 m_block_size;
    usize *const m_block_bitmap;
    uint8 *const m_block_table;

    ufs::InodeHeader *header_ptr(ufs::BlockIndex index) {
        return reinterpret_cast<ufs::InodeHeader *>(&m_block_table[index * m_block_size]);
    }

public:
    explicit BlockGroup(uint8 *&data, uint16 block_size)
        : m_block_size(block_size), m_block_bitmap(reinterpret_cast<usize *>(data)), m_block_table(data) {
        m_block_bitmap[0] |= 1u;
        data += ufs::blocks_per_group(block_size) * block_size;
    }

    ufs::BlockIndex allocate_block();
    Inode allocate_inode(ufs::InodeType type);
    uint16 block_size() const { return m_block_size; }
};

ufs::BlockIndex BlockGroup::allocate_block() {
    for (usize i = 0; i < m_block_size / sizeof(usize); i++) {
        const usize bucket = m_block_bitmap[i];
        if (bucket == ustd::Limits<usize>::max()) {
            continue;
        }
        const usize bucket_block = ustd::cto(bucket);
        m_block_bitmap[i] |= (1u << bucket_block);
        return i * 8 * sizeof(usize) + bucket_block;
    }
    ENSURE_NOT_REACHED("Block group out of blocks");
}

Inode BlockGroup::allocate_inode(ufs::InodeType type) {
    auto block_index = allocate_block();
    auto *header = header_ptr(block_index);
    header->type = type;
    return {*this, block_index, header};
}

void Inode::write(ustd::Span<const void> data) const {
    const usize max_size = m_block_group.block_size() - sizeof(ufs::InodeHeader);
    if (data.size() > max_size) {
        data = {data.data(), max_size};
        std::fprintf(stderr, "WARN: too big\n");
    }
    m_header->size = data.size();
    std::memcpy(m_header + 1, data.data(), data.size());
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <image> <sysroot>\n", argv[0]);
        return 1;
    }

    usize data_size = 0;
    for (const auto &child : std::filesystem::recursive_directory_iterator(argv[2])) {
        data_size += sizeof(ufs::DirectoryEntry);
        data_size += 4_KiB;
        if (!child.is_directory()) {
            data_size += child.file_size();
            std::printf("%s %lu\n", child.path().c_str(), child.file_size());
        }
    }

    constexpr usize k_block_size = 4_KiB;
    usize total_size = data_size + 2 * k_block_size;

    int fd = open(argv[1], O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (fd < 0) {
        std::perror("creat");
        return 1;
    }
    if (ftruncate(fd, static_cast<off_t>(total_size)) < 0) {
        std::perror("ftruncate");
        return 1;
    }
    auto *data = static_cast<uint8 *>(mmap(nullptr, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    if (data == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }

    ufs::Superblock superblock{
        .magic = ufs::k_superblock_magic,
        .block_count = ustd::ceil_div(data_size, k_block_size),
        .block_size = k_block_size,
    };
    std::memcpy(data, &superblock, sizeof(ufs::Superblock));
    data += superblock.block_size;

    uint32 block_group_count = ustd::ceil_div(superblock.block_count, ufs::blocks_per_group(superblock.block_size));
    for (uint32 i = 0; i < block_group_count; i++) {
        ufs::GroupHeader group_header{
            .block_bitmap = 2,
            .block_table = 3,
        };
        std::memcpy(data, &group_header, sizeof(ufs::GroupHeader));
        data += sizeof(ufs::GroupHeader);
    }
    data = reinterpret_cast<uint8 *>(ustd::round_up(reinterpret_cast<uintptr>(data), superblock.block_size));

    ustd::Vector<BlockGroup> groups(block_group_count, data, superblock.block_size);
    ustd::Function<void(const Inode &, const std::filesystem::path &)> iterate_directory =
        [&](const Inode &inode, const std::filesystem::path &path) {
            ustd::Vector<ufs::DirectoryEntry> entries;
            for (const auto &child : std::filesystem::directory_iterator(path)) {
                auto name = child.path().filename().string();
                auto &entry = entries.emplace(ufs::DirectoryEntry{
                    .name_length = name.length(),
                });
                std::memcpy(static_cast<char *>(entry.name), name.c_str(), name.length());
                if (child.is_directory()) {
                    auto child_inode = groups[0].allocate_inode(ufs::InodeType::Directory);
                    entry.inode = child_inode.inode_index();
                    iterate_directory(child_inode, child.path());
                }
            }
            inode.write(entries.span());
        };

    auto root_inode = groups[0].allocate_inode(ufs::InodeType::Directory);
    iterate_directory(root_inode, argv[2]);
}
