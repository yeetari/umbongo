#include <ustd/string_builder.hh>

#include <ustd/span.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/vector.hh>

namespace ustd {

void StringBuilder::append_single(const char *, const char *arg) {
    while (*arg != '\0') {
        m_buffer.push(*arg++);
    }
}

void StringBuilder::append_single(const char *, StringView arg) {
    m_buffer.ensure_capacity(m_buffer.size() + arg.length());
    for (char ch : arg) {
        m_buffer.push(ch);
    }
}

void StringBuilder::append_single(const char *, bool arg) {
    append_single("", arg ? "true" : "false");
}

void StringBuilder::append(char ch) {
    m_buffer.push(ch);
}

String StringBuilder::build() {
    m_buffer.push('\0');
    auto buffer = m_buffer.take_all();
    return String::move_raw(buffer.data(), buffer.size() - 1);
}

String StringBuilder::build_copy() const {
    return String::copy_raw(m_buffer.data(), m_buffer.size());
}

} // namespace ustd
