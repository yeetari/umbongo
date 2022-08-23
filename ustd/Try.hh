#pragma once

// TODO: ASSUME macro like EXPECT but that ASSERTs instead?
#define EXPECT(expr, ...)                                                                                              \
    ({                                                                                                                 \
        auto _result_expect = (expr);                                                                                  \
        ENSURE(_result_expect.has_value() __VA_OPT__(, ) __VA_ARGS__);                                                 \
        _result_expect.disown_value();                                                                                 \
    })

#define TRY(expr)                                                                                                      \
    ({                                                                                                                 \
        auto _result_try = (expr);                                                                                     \
        if (!_result_try.has_value()) {                                                                                \
            return _result_try.error();                                                                                \
        }                                                                                                              \
        _result_try.disown_value();                                                                                    \
    })
