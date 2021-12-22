#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

namespace kernel {

enum class PageFlags : usize {
    Present = 1ul << 0ul,
    Writable = 1ul << 1ul,
    User = 1ul << 2ul,
    WriteThrough = 1ul << 3ul,
    CacheDisable = 1ul << 4ul,
    Large = 1ul << 7ul,
    NoExecute = 1ul << 63ul,
};

inline constexpr PageFlags operator&(PageFlags a, PageFlags b) {
    return static_cast<PageFlags>(static_cast<usize>(a) & static_cast<usize>(b));
}

inline constexpr PageFlags operator|(PageFlags a, PageFlags b) {
    return static_cast<PageFlags>(static_cast<usize>(a) | static_cast<usize>(b));
}

inline constexpr PageFlags &operator|=(PageFlags &a, PageFlags b) {
    return a = (a | b);
}

template <typename Entry>
class [[gnu::packed]] PageLevelEntry {
    uintptr m_raw{0};

public:
    constexpr PageLevelEntry() = default;
    constexpr PageLevelEntry(uintptr entry, PageFlags flags)
        : m_raw((entry & 0xfffffffffffffu) | static_cast<usize>(flags | PageFlags::Present)) {}

    bool empty() const { return m_raw == 0; }
    Entry *entry() const { return reinterpret_cast<Entry *>(m_raw & 0xffffffffff000ul); }
    PageFlags flags() const { return static_cast<PageFlags>(m_raw & 0xffu); }
};

template <typename Entry>
class [[gnu::aligned(4_KiB)]] PageLevel {
    ustd::Array<PageLevelEntry<Entry>, 512> m_entries{};

public:
    constexpr PageLevel() = default;
    PageLevel(const PageLevel &) = delete;
    PageLevel(PageLevel &&) = delete;
    ~PageLevel() {
        if constexpr (!ustd::IsSame<Entry, uintptr>) {
            for (auto &entry : m_entries) {
                if (!entry.empty() && (entry.flags() & PageFlags::Large) != PageFlags::Large) {
                    delete entry.entry();
                }
            }
        }
    }

    PageLevel &operator=(const PageLevel &) = delete;
    PageLevel &operator=(PageLevel &&) = delete;

    void set(usize index, uintptr phys, PageFlags flags = static_cast<PageFlags>(0)) {
        ASSERT(index < 512);
        ASSERT(m_entries[index].empty());
        m_entries[index] = PageLevelEntry<Entry>(phys, flags);
    }

    Entry *ensure(usize index) {
        ASSERT(index < 512);
        ASSERT((m_entries[index].flags() & PageFlags::Large) != PageFlags::Large);
        if (m_entries[index].empty()) {
            // Top level page structures have all access bits set.
            set(index, reinterpret_cast<uintptr>(new Entry), PageFlags::Writable | PageFlags::User);
        }
        return m_entries[index].entry();
    }

    const ustd::Array<PageLevelEntry<Entry>, 512> &entries() const { return m_entries; }
};

using Page = uintptr;                                   // PTE
using PageTable = PageLevel<Page>;                      // PDE
using PageDirectory = PageLevel<PageTable>;             // PDPE
using PageDirectoryPtrTable = PageLevel<PageDirectory>; // PML4E
using Pml4 = PageLevel<PageDirectoryPtrTable>;

} // namespace kernel
