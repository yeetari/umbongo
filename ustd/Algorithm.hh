#pragma once

#include <ustd/Concepts.hh>
#include <ustd/Memory.hh>
#include <ustd/Traits.hh>
#include <ustd/Utility.hh>

namespace ustd {

// clang-format off
template <typename T>
concept Container = requires(const T &container) {
    { container.data() } -> ConvertibleTo<const void *>;
    { container.size() } -> Integral;
    { container.size_bytes() } -> Integral;
};
// clang-format on

template <Container Container, typename T = RemoveRef<decltype(*declval<Container>().data())>>
constexpr bool equal(const Container &a, const Container &b) {
    if (a.size() != b.size()) {
        return false;
    }
    if constexpr (IsTriviallyCopyable<T>) {
        return memcmp(a.data(), b.data(), b.size_bytes()) == 0;
    }
    for (usize i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

template <Container Container, typename T>
constexpr void fill(Container &container, const T &value) {
    if constexpr (IsTriviallyCopyable<T>) {
        for (auto *elem = container.data(); elem < container.data() + container.size(); elem++) {
            *elem = value;
        }
        return;
    }
    for (auto *elem = container.data(); elem < container.data() + container.size(); elem++) {
        new (elem) T(value);
    }
}

} // namespace ustd
