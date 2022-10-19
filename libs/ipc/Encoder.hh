#pragma once

#include <ustd/Assert.hh>
#include <ustd/Concepts.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>
#include <ustd/Traits.hh>
#include <ustd/Types.hh>

namespace ipc {

class Encoder {
    ustd::Vector<uint8_t> m_buffer;

public:
    template <ustd::TriviallyCopyable T>
    void encode(const T &obj);
    void encode(ustd::StringView view);

    ustd::Span<const uint8_t> span() const { return m_buffer.span(); }
};

template <ustd::TriviallyCopyable T>
void Encoder::encode(const T &obj) {
    m_buffer.ensure_size(m_buffer.size() + sizeof(T));
    __builtin_memcpy(m_buffer.end() - sizeof(T), &obj, sizeof(T));
}

inline void Encoder::encode(ustd::StringView view) {
    encode(view.length());
    m_buffer.ensure_size(m_buffer.size() + view.length());
    __builtin_memcpy(m_buffer.end() - view.length(), view.data(), view.length());
}

} // namespace ipc
