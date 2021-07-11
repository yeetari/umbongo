#pragma once

#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace ustd {

class StringView : public Span<const char> {
public:
    using Span::Span;
    constexpr StringView(const char *string) : Span(string, __builtin_strlen(string)) {}

    constexpr bool empty() const { return length() == 0; }
    constexpr usize length() const { return size(); }
};

} // namespace ustd

using ustd::StringView;
