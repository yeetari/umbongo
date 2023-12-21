#include <ustd/string.hh>

#include <ustd/assert.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace ustd {

String String::copy_raw(const char *data, size_t length) {
    String string(length);
    __builtin_memcpy(string.m_data, data, length);
    return string;
}

String String::move_raw(char *data, size_t length) {
    ASSERT(data[length] == '\0');
    String string;
    string.m_data = data;
    string.m_length = length;
    return string;
}

String::String(size_t length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
}

String::~String() {
    delete[] m_data;
}

String &String::operator=(String &&other) {
    m_data = ustd::exchange(other.m_data, nullptr);
    m_length = ustd::exchange(other.m_length, 0ul);
    return *this;
}

char String::operator[](size_t index) const {
    ASSERT(index < m_length);
    return m_data[index];
}

} // namespace ustd
