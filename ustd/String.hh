#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

class String {
    char *m_data{nullptr};
    size_t m_length{0};

public:
    static String copy_raw(const char *data, size_t length);
    static String move_raw(char *data, size_t length);

    String() = default;
    String(const char *data) : String(copy_raw(data, __builtin_strlen(data))) {}
    explicit String(size_t length);
    explicit String(StringView view) : String(copy_raw(view.data(), view.length())) {}
    String(const String &other) : String(copy_raw(other.m_data, other.m_length)) {}
    String(String &&other) : m_data(exchange(other.m_data, nullptr)), m_length(exchange(other.m_length, 0u)) {}
    ~String();

    // TODO: Implement these.
    String &operator=(const String &) = delete;
    String &operator=(String &&);

    char operator[](size_t index) const;

    const char *begin() const { return m_data; }
    const char *end() const { return m_data + m_length; }
    operator StringView() const { return {m_data, m_length}; }

    bool empty() const { return m_length == 0; }
    char *data() { return m_data; }
    const char *data() const { return m_data; }
    size_t length() const { return m_length; }
};

} // namespace ustd
