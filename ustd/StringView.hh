#pragma once

#include <ustd/Memory.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ustd {

class StringView : public Span<const char> {
public:
    using Span::Span;
    constexpr StringView(const char *string) : Span(string, __builtin_strlen(string)) {}

    constexpr bool operator==(const char *string) const;
    constexpr bool operator==(StringView other) const;

    constexpr bool empty() const { return length() == 0; }
    constexpr usize length() const { return size(); }
};

constexpr bool StringView::operator==(const char *string) const {
    if (data() == nullptr) {
        return string == nullptr;
    }
    if (string == nullptr) {
        return false;
    }
    const char *ch = string;
    for (usize i = 0; i < length(); i++) {
        if (*ch == '\0' || data()[i] != *(ch++)) {
            return false;
        }
    }
    return *ch == '\0';
}

constexpr bool StringView::operator==(StringView other) const {
    if (data() == nullptr) {
        return other.data() == nullptr;
    }
    if (other.data() == nullptr) {
        return false;
    }
    if (length() != other.length()) {
        return false;
    }
    return memcmp(data(), other.data(), length()) == 0;
}

} // namespace ustd

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-literals"
constexpr ustd::StringView operator"" sv(const char *string, usize length) {
    return {string, length};
}
#pragma clang diagnostic pop
