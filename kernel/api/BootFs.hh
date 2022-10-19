#pragma once

#include <ustd/Optional.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

class BootFsEntry {
    size_t m_data_size{0};
    uint16_t m_name_length{0};

public:
    BootFsEntry(size_t data_size, uint16_t name_length) : m_data_size(data_size), m_name_length(name_length) {}
    BootFsEntry(const BootFsEntry &) = delete;
    BootFsEntry(BootFsEntry &&) = delete;
    ~BootFsEntry() = default;

    BootFsEntry &operator=(const BootFsEntry &) = delete;
    BootFsEntry &operator=(BootFsEntry &&) = delete;

    ustd::Optional<const BootFsEntry &> find(ustd::StringView name) {
        for (const auto *entry = this; entry != nullptr; entry = entry->next_entry()) {
            if (entry->name() == name) {
                return *entry;
            }
        }
        return {};
    }

    ustd::Span<const uint8_t> data() const {
        return {reinterpret_cast<const uint8_t *>(name().data()) + m_name_length, m_data_size};
    }
    ustd::StringView name() const { return {reinterpret_cast<const char *>(this + 1), m_name_length}; }
    const BootFsEntry *next_entry() const {
        const auto *next = reinterpret_cast<const BootFsEntry *>(data().byte_offset(m_data_size));
        if (next->m_data_size == size_t(-1)) {
            return nullptr;
        }
        return next;
    }
};
