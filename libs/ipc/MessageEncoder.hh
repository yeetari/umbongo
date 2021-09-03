#pragma once

#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ipc {

class MessageEncoder {
    const Span<uint8> m_buffer;
    uint8 *m_ptr;
    usize m_size{0};

public:
    explicit MessageEncoder(Span<uint8> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <TriviallyCopyable T>
    void encode(const T &obj);

    usize size() const { return m_size; }
};

template <TriviallyCopyable T>
void MessageEncoder::encode(const T &obj) {
    ASSERT(m_size + sizeof(T) < m_buffer.size());
    memcpy(m_ptr + m_size, &obj, sizeof(T));
    m_size += sizeof(T);
}

} // namespace ipc
