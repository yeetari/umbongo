#pragma once

#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ustd {

class StringView : public Span<const char> {
public:
    using Span::Span;
    constexpr StringView(const char *string) : Span(string, __builtin_strlen(string)) {}

    StringView substr(usize begin) const;
    StringView substr(usize begin, usize end) const;
    bool operator==(const char *string) const;
    bool operator==(StringView other) const;

    constexpr bool empty() const { return length() == 0; }
    constexpr usize length() const { return size(); }
};

} // namespace ustd

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wuser-defined-literals"
constexpr ustd::StringView operator"" sv(const char *string, usize length) {
    return {string, length};
}
#pragma clang diagnostic pop
