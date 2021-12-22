#pragma once

#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Span.hh>

namespace ipc {

class MessageDecoder {
    const ustd::Span<const uint8> m_buffer;
    const uint8 *m_ptr;

public:
    explicit MessageDecoder(ustd::Span<const uint8> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <ustd::TriviallyCopyable T>
    T decode();

    usize bytes_decoded() const { return static_cast<usize>(m_ptr - m_buffer.data()); }
};

template <ustd::TriviallyCopyable T>
T MessageDecoder::decode() {
    ASSERT(m_ptr + sizeof(T) <= m_buffer.data() + m_buffer.size());
    T ret;
    __builtin_memcpy(&ret, m_ptr, sizeof(T));
    m_ptr += sizeof(T);
    return ret;
}

} // namespace ipc
