#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

class String {
    char *m_data{nullptr};
    usize m_length{0};

public:
    static String copy_raw(const char *data, usize length);
    static String move_raw(char *data, usize length);

    String() = default;
    String(const char *data) : String(copy_raw(data, __builtin_strlen(data))) {}
    explicit String(usize length);
    explicit String(StringView view) : String(copy_raw(view.data(), view.length())) {}
    String(const String &other) : String(copy_raw(other.m_data, other.m_length)) {}
    String(String &&other) noexcept : m_data(exchange(other.m_data, nullptr)), m_length(exchange(other.m_length, 0u)) {}
    ~String();

    // TODO: Implement these.
    String &operator=(const String &) = delete;
    String &operator=(String &&) noexcept;

    char operator[](usize index) const;

    const char *begin() const { return m_data; }
    const char *end() const { return m_data + m_length; }
    StringView view() const { return {m_data, m_length}; }

    bool empty() const { return m_length == 0; }
    char *data() { return m_data; }
    const char *data() const { return m_data; }
    usize length() const { return m_length; }
};

} // namespace ustd
