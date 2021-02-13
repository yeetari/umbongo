#pragma once

namespace ustd {

template <typename Container, typename T>
constexpr void fill(Container &container, const T &value) {
    // TODO: Concept constraints.
    // TODO: Specialization for integral types using memset?
    for (auto *elem = container.data(); elem < container.data() + container.size(); elem++) {
        *elem = value;
    }
}

} // namespace ustd
