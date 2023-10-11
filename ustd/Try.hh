#pragma once

namespace ustd::detail {

template <typename T>
concept IsRefWrapper = requires { T::k_is_ref_wrapper; };

template <bool>
struct Selector;

template <>
struct Selector<false> {
    [[gnu::always_inline]] static auto select(auto obj) { return obj; }
};

template <>
struct Selector<true> {
    [[gnu::always_inline]] static decltype(auto) select(auto &&wrapper) { return wrapper.ref(); }
};

} // namespace ustd::detail

#define ASSUME_INTERNAL(expr, ...)                                                                                     \
    ({                                                                                                                 \
        auto _result_assume = (expr);                                                                                  \
        ASSERT(_result_assume.has_value() __VA_OPT__(, ) __VA_ARGS__);                                                 \
        _result_assume.disown_value();                                                                                 \
    })

#define EXPECT_INTERNAL(expr, ...)                                                                                     \
    ({                                                                                                                 \
        auto _result_expect = (expr);                                                                                  \
        ENSURE(_result_expect.has_value() __VA_OPT__(, ) __VA_ARGS__);                                                 \
        _result_expect.disown_value();                                                                                 \
    })

#define TRY_INTERNAL(expr)                                                                                             \
    ({                                                                                                                 \
        auto _result_try = (expr);                                                                                     \
        if (!_result_try.has_value()) {                                                                                \
            return _result_try.error();                                                                                \
        }                                                                                                              \
        _result_try.disown_value();                                                                                    \
    })

#define SELECTOR_DISPATCH(macro, expr, ...)                                                                            \
    ustd::detail::Selector<ustd::detail::IsRefWrapper<decltype(macro(expr __VA_OPT__(, ) __VA_ARGS__))>>::select(      \
        macro(expr __VA_OPT__(, ) __VA_ARGS__))
#define ASSUME(expr, ...) SELECTOR_DISPATCH(ASSUME_INTERNAL, expr __VA_OPT__(, ) __VA_ARGS__)
#define EXPECT(expr, ...) SELECTOR_DISPATCH(EXPECT_INTERNAL, expr __VA_OPT__(, ) __VA_ARGS__)
#define TRY(expr) SELECTOR_DISPATCH(TRY_INTERNAL, expr)
