#include <ustd/string_view.hh>

#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace ustd {

StringView StringView::substr(size_t begin) const {
    return substr(begin, length());
}

StringView StringView::substr(size_t begin, size_t end) const {
    ASSERT(begin + (end - begin) <= size());
    return {data() + begin, end - begin};
}

bool StringView::operator==(const char *string) const {
    if (data() == nullptr) {
        return string == nullptr;
    }
    if (string == nullptr) {
        return false;
    }
    const char *ch = string;
    for (size_t i = 0; i < length(); i++) {
        if (*ch == '\0' || data()[i] != *(ch++)) {
            return false;
        }
    }
    return *ch == '\0';
}

bool StringView::operator==(StringView other) const {
    if (data() == nullptr) {
        return other.data() == nullptr;
    }
    if (other.data() == nullptr) {
        return false;
    }
    if (length() != other.length()) {
        return false;
    }
    return __builtin_memcmp(data(), other.data(), length()) == 0;
}

} // namespace ustd
