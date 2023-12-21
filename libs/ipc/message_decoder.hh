#pragma once

#include <ustd/assert.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ipc {

class MessageDecoder {
    const ustd::Span<const uint8_t> m_buffer;
    const uint8_t *m_ptr;

public:
    explicit MessageDecoder(ustd::Span<const uint8_t> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <ustd::TriviallyCopyable T>
    T decode();

    size_t bytes_decoded() const { return static_cast<size_t>(m_ptr - m_buffer.data()); }
};

template <ustd::TriviallyCopyable T>
T MessageDecoder::decode() {
    ASSERT(m_ptr + sizeof(T) <= m_buffer.data() + m_buffer.size());
    T ret;
    __builtin_memcpy(&ret, m_ptr, sizeof(T));
    m_ptr += sizeof(T);
    return ret;
}

template <>
inline ustd::StringView MessageDecoder::decode() {
    auto length = decode<size_t>();
    ustd::StringView view(reinterpret_cast<const char *>(m_ptr), length);
    m_ptr += length;
    return view;
}

} // namespace ipc
