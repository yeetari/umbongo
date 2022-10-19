#pragma once

#include <ustd/Types.hh>
#include <ustd/Utility.hh>

namespace ustd {

template <typename I, I... Is>
struct IntegerSequence {};

template <size_t... Is>
using IndexSequence = IntegerSequence<size_t, Is...>;

template <typename I, I N>
using make_integer_sequence = __make_integer_seq<IntegerSequence, I, N>;

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;

template <size_t>
struct TupleTag {};

template <size_t I, typename T>
struct TupleElem {
    static T elem_type(TupleTag<I>);
    [[no_unique_address]] T value; // NOLINT: must be public
    constexpr decltype(auto) operator[](TupleTag<I>) & { return (value); }
};

template <typename... Ts>
struct TupleMap : Ts... {
    using Ts::elem_type...;
    using Ts::operator[]...;
};

template <typename, typename...>
struct TupleBase;
template <size_t... Is, typename... Ts>
struct TupleBase<IndexSequence<Is...>, Ts...> : TupleMap<TupleElem<Is, Ts>...> {};

template <typename... Ts>
struct Tuple : TupleBase<make_index_sequence<sizeof...(Ts)>, Ts...> {};

template <typename... Ts>
Tuple(Ts...) -> Tuple<decay<Ts>...>;

template <size_t I, typename T>
constexpr decltype(auto) get(T &&tuple) {
    return tuple[TupleTag<I>()];
}

template <typename... Ts>
constexpr auto make_tuple(Ts &&...args) {
    return Tuple<decay<Ts>...>{forward<Ts>(args)...};
}

} // namespace ustd

namespace std {

template <size_t I, typename T>
struct tuple_element;
template <typename T>
struct tuple_size;

template <size_t I, typename... Ts>
struct tuple_element<I, ustd::Tuple<Ts...>> {
    using type = decltype(ustd::Tuple<Ts...>::elem_type(ustd::TupleTag<I>()));
};

template <typename... Ts>
struct tuple_size<ustd::Tuple<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
};

} // namespace std
