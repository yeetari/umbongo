#pragma once

#include <kernel/acpi/table.hh>
#include <ustd/algorithm.hh>
#include <ustd/optional.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

class RootTableIterator;

// Extended System Description Table (ACPI specification 6.4 section 5.2.8)
class [[gnu::packed]] RootTable : public Table {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#pragma clang diagnostic ignored "-Wgnu-empty-struct"
    // NOLINTNEXTLINE
    const Table *m_entries[];
#pragma clang diagnostic pop

public:
    RootTableIterator begin() const;
    RootTableIterator end() const;

    template <typename T>
    ustd::Optional<const T *> find() const;

    const Table *entry(size_t index) const;
    size_t entry_count() const;
};

class RootTableIterator {
    const RootTable *const m_table;
    size_t m_index;

public:
    constexpr RootTableIterator(const RootTable *table, size_t index) : m_table(table), m_index(index) {}

    constexpr RootTableIterator &operator++() {
        m_index++;
        return *this;
    }

    constexpr RootTableIterator &operator--() {
        m_index--;
        return *this;
    }

    constexpr bool operator<=>(const RootTableIterator &) const = default;
    constexpr const Table *operator*() const { return m_table->entry(m_index); }
};

inline RootTableIterator RootTable::begin() const {
    return {this, 0};
}

inline RootTableIterator RootTable::end() const {
    return {this, entry_count()};
}

template <typename T>
ustd::Optional<const T *> RootTable::find() const {
    for (const auto *table : *this) {
        if (ustd::equal(table->signature(), T::k_signature) && table->valid()) {
            return static_cast<const T *>(table);
        }
    }
    return {};
}

} // namespace kernel::acpi
