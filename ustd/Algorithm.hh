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

template <Container Container, typename GreaterThan, typename SizeType = decltype(declval<Container>().size())>
constexpr void sort(Container &container, GreaterThan gt) {
    if (container.size() == 0) {
        return;
    }
    auto gap = container.size();
    bool swapped = false;
    do {
        gap = (gap * 10) / 13;
        if (gap == 9 || gap == 10) {
            gap = 11;
        }
        if (gap < 1) {
            gap = 1;
        }
        swapped = false;
        for (SizeType i = 0; i < container.size() - gap; i++) {
            SizeType j = i + gap;
            if (gt(container[i], container[j])) {
                swap(container[i], container[j]);
                swapped = true;
            }
        }
    } while (gap > 1 || swapped);
}

template <Container Container>
constexpr void sort(Container &container) {
    sort(container, [](const auto &lhs, const auto &rhs) {
        return lhs > rhs;
    });
}

} // namespace ustd
