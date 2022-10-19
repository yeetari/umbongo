#pragma once

#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh> // TODO: Remove.

namespace ipc {

enum class DecoderError {
    BufferOverflow,
};

class Decoder {
    const ustd::Span<const uint8_t> m_buffer;
    const uint8_t *m_ptr;

public:
    explicit Decoder(ustd::Span<const uint8_t> buffer) : m_buffer(buffer), m_ptr(buffer.data()) {}

    template <ustd::TriviallyCopyable T>
    ustd::Result<T, DecoderError> decode();

    size_t bytes_decoded() const { return static_cast<size_t>(m_ptr - m_buffer.data()); }
};

template <ustd::TriviallyCopyable T>
ustd::Result<T, DecoderError> Decoder::decode() {
    if (m_ptr + sizeof(T) > m_buffer.end()) {
        return DecoderError::BufferOverflow;
    }
    T ret;
    __builtin_memcpy(&ret, m_ptr, sizeof(T));
    m_ptr += sizeof(T);
    return ret;
}

// TODO: Move to source file.
template <>
inline ustd::Result<ustd::StringView, DecoderError> Decoder::decode() {
    const auto length = TRY(decode<size_t>());
    if (m_ptr + length > m_buffer.end()) {
        return DecoderError::BufferOverflow;
    }
    ustd::StringView view(reinterpret_cast<const char *>(m_ptr), length);
    m_ptr += length;
    return view;
}

} // namespace ipc
