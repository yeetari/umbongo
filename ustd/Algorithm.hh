#pragma once

#include <ustd/Concepts.hh>
#include <ustd/Optional.hh>
#include <ustd/Traits.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
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

template <Container Container, typename T = remove_ref<decltype(*declval<Container>().data())>>
constexpr bool equal(const Container &a, const Container &b) {
    if (a.size() != b.size()) {
        return false;
    }
    if constexpr (is_trivially_copyable<T>) {
        return __builtin_memcmp(a.data(), b.data(), b.size_bytes()) == 0;
    }
    for (size_t i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

template <Container Container, typename T>
constexpr void fill(Container &container, const T &value) {
    if constexpr (is_trivially_copyable<T>) {
        for (auto *elem = container.data(); elem < container.data() + container.size(); elem++) {
            *elem = value;
        }
        return;
    }
    for (auto *elem = container.data(); elem < container.data() + container.size(); elem++) {
        new (elem) T(value);
    }
}

template <typename It, typename T>
constexpr void fill_n(It it, size_t size, T value) {
    for (size_t i = 0; i < size; i++) {
        *it++ = static_cast<remove_ref<decltype(*it)>>(value);
    }
}

// TODO: Concept constraints.
template <typename Container, typename T>
constexpr ustd::Optional<size_t> index_of(const Container &container, const T &value) {
    for (size_t i = 0; i < container.size(); i++) {
        if (container[i] == value) {
            return i;
        }
    }
    return {};
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
