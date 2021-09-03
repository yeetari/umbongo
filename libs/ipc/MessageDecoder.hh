#pragma once

#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Span.hh>

namespace ipc {

class MessageDecoder {
    const Span<const uint8> m_buffer;
    const uint8 *m_ptr;

public:
    explicit MessageDecoder(Span<const uint8> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <TriviallyCopyable T>
    T decode();
};

template <TriviallyCopyable T>
T MessageDecoder::decode() {
    ASSERT(m_ptr + sizeof(T) <= m_buffer.data() + m_buffer.size());
    T ret;
    memcpy(&ret, m_ptr, sizeof(T));
    m_ptr += sizeof(T);
    return ret;
}

} // namespace ipc
