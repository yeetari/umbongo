#pragma once

namespace ustd {
namespace detail {

template <bool B, typename T, typename F>
struct Conditional {
    using type = T;
};
template <typename T, typename F>
struct Conditional<false, T, F> {
    using type = F;
};

template <typename, typename U>
struct CopyConst {
    using type = U;
};
template <typename T, typename U>
struct CopyConst<const T, U> {
    using type = const U;
};

template <typename T>
struct RemoveCv {
    using type = T;
};
template <typename T>
struct RemoveCv<const T> {
    using type = T;
};
template <typename T>
struct RemoveCv<volatile T> {
    using type = T;
};
template <typename T>
struct RemoveCv<const volatile T> {
    using type = T;
};

template <typename T>
struct RemoveRef {
    using type = T;
};
template <typename T>
struct RemoveRef<T &> {
    using type = T;
};
template <typename T>
struct RemoveRef<T &&> {
    using type = T;
};

} // namespace detail

template <bool B, typename T, typename F>
using conditional = typename detail::Conditional<B, T, F>::type;
template <typename T, typename U>
using copy_const = typename detail::CopyConst<T, U>::type;
template <typename T>
using remove_cv = typename detail::RemoveCv<T>::type;
template <typename T>
using remove_ref = typename detail::RemoveRef<T>::type;

// TODO: Actually do the handling for arrays and functions.
template <typename T>
using decay = remove_cv<remove_ref<T>>;

template <typename T, typename U>
inline constexpr bool is_base_of = __is_base_of(T, U);

template <typename>
inline constexpr bool is_const = false;
template <typename T>
inline constexpr bool is_const<const T> = true;

template <typename T>
inline constexpr bool is_enum = __is_enum(T);
template <typename>
inline constexpr bool is_pointer = false;
template <typename T>
inline constexpr bool is_pointer<T *> = true;

template <typename>
inline constexpr bool is_reference = false;
template <typename T>
inline constexpr bool is_reference<T &> = true;
template <typename T>
inline constexpr bool is_reference<T &&> = true;

template <typename, typename>
inline constexpr bool is_same = false;
template <typename T>
inline constexpr bool is_same<T, T> = true;

template <typename T>
inline constexpr bool is_trivially_constructible = __is_trivially_constructible(T);
template <typename T>
inline constexpr bool is_trivially_copyable = __is_trivially_copyable(T);
template <typename T>
inline constexpr bool is_trivially_destructible = __is_trivially_destructible(T);

template <typename T, typename U>
inline constexpr bool is_convertible_to = is_same<T, U> || requires(T obj) { static_cast<U>(obj); };

template <typename T, typename U>
concept ConvertibleTo = is_convertible_to<T, U>;

template <typename T>
concept Enum = is_enum<T>;

template <typename T>
concept Pointer = is_pointer<T>;

template <typename T, typename U>
concept SameAs = is_same<T, U>;

template <typename T>
concept TriviallyCopyable = is_trivially_copyable<T>;

template <typename E>
using underlying_type = __underlying_type(E);

template <typename E>
constexpr auto to_underlying(E value) {
    return static_cast<underlying_type<E>>(value);
}

template <typename T>
T declval();

template <typename T>
constexpr T &&forward(remove_ref<T> &arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr T &&forward(remove_ref<T> &&arg) {
    return static_cast<T &&>(arg);
}

template <typename T>
constexpr remove_ref<T> &&move(T &&arg) {
    return static_cast<remove_ref<T> &&>(arg);
}

template <typename T, typename U = T>
constexpr T exchange(T &obj, U &&new_value) {
    T old_value = move(obj);
    obj = forward<U>(new_value);
    return old_value;
}

template <typename T>
constexpr void swap(T &lhs, T &rhs) {
    T tmp(move(lhs));
    lhs = move(rhs);
    rhs = move(tmp);
}

template <TriviallyCopyable To, TriviallyCopyable From>
constexpr To bit_cast(const From &from) {
    static_assert(sizeof(To) == sizeof(From));
    return __builtin_bit_cast(To, from);
}

[[noreturn]] inline void unreachable() {
    __builtin_unreachable();
}

template <typename T>
inline constexpr bool is_move_constructible = requires(T &&t) { new T(move(t)); };

} // namespace ustd

inline void *operator new(__SIZE_TYPE__, void *ptr) {
    return ptr;
}

inline void *operator new[](__SIZE_TYPE__, void *ptr) {
    return ptr;
}
