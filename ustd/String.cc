#include <ustd/String.hh>

#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace ustd {

String::String(const char *data, usize length) : m_length(length) {
    m_data = new char[length + 1];
    m_data[length] = '\0';
    memcpy(m_data, data, length);
}

String::~String() {
    delete[] m_data;
}

} // namespace ustd
