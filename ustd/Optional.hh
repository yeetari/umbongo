#pragma once

#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename T>
class Optional {
    Array<uint8, sizeof(T)> m_data{};
    bool m_present{false};

public:
    constexpr Optional() = default;
    constexpr Optional(const T &value) : m_present(true) { new (m_data.data()) T(value); }  // NOLINT
    constexpr Optional(T &&value) : m_present(true) { new (m_data.data()) T(move(value)); } // NOLINT

    constexpr explicit operator bool() const noexcept { return m_present; }
    constexpr const T &operator*() const {
        ASSERT(m_present);
        return *reinterpret_cast<const T *>(m_data.data());
    }
};

} // namespace ustd

using ustd::Optional;
