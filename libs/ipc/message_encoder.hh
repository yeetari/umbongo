#pragma once

#include <ustd/assert.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ipc {

class MessageEncoder {
    const ustd::Span<uint8_t> m_buffer;
    uint8_t *m_ptr;
    size_t m_size{0};

public:
    explicit MessageEncoder(ustd::Span<uint8_t> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <ustd::TriviallyCopyable T>
    void encode(const T &obj);
    void encode(ustd::StringView view);

    size_t size() const { return m_size; }
};

template <ustd::TriviallyCopyable T>
void MessageEncoder::encode(const T &obj) {
    ASSERT(m_size + sizeof(T) < m_buffer.size());
    __builtin_memcpy(m_ptr + m_size, &obj, sizeof(T));
    m_size += sizeof(T);
}

inline void MessageEncoder::encode(ustd::StringView view) {
    encode(view.length());
    ASSERT(m_size + view.length() < m_buffer.size());
    __builtin_memcpy(m_ptr + m_size, view.data(), view.length());
    m_size += view.length();
}

} // namespace ipc
