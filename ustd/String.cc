#include <ustd/String.hh>

#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

String::String(const char *data, usize length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
    memcpy(m_data, data, length);
}

String::String(usize length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
}

String::~String() {
    delete[] m_data;
}

String &String::operator=(String &&other) noexcept {
    m_data = ustd::exchange(other.m_data, nullptr);
    m_length = ustd::exchange(other.m_length, 0ul);
    return *this;
}

char String::operator[](usize index) const {
    ASSERT(index < m_length);
    return m_data[index];
}

} // namespace ustd
