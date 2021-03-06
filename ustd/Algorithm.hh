#pragma once

#include <ustd/Memory.hh>
#include <ustd/Traits.hh>

namespace ustd {

template <typename Container>
concept IsContainer = requires(const Container &container) {
    { container.data() } -> IsConvertibleTo<const void *>;
    { container.size() } -> IsSame<usize>;
};

// TODO: Find a way to use "template <Container<T> ContainerA>" syntax.
#define CONTAINER_1                                                                                                    \
    template <typename T, auto... Ts, template <typename, auto...> typename Container>                                 \
    requires IsContainer<Container<T, Ts...>>
#define CONTAINER_2                                                                                                    \
    template <typename T, auto... Ts, template <typename, auto...> typename ContainerA, typename U, auto... Us,        \
              template <typename, auto...> typename ContainerB>                                                        \
    requires IsContainer<ContainerA<T, Ts...>> && IsContainer<ContainerB<U, Us...>>

CONTAINER_2
constexpr bool equal(const ContainerA<T, Ts...> &a, const ContainerB<U, Us...> &b) {
    if (a.size() != b.size()) {
        return false;
    }
    if constexpr (IsTriviallyCopyable<T> && IsTriviallyCopyable<U>) {
        return memcmp(a.data(), b.data(), b.size()) == 0;
    }
    for (usize i = 0; i < a.size(); i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

CONTAINER_1
constexpr void fill(Container<T, Ts...> &container, const T &value) {
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
