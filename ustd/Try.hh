#pragma once

// TODO: ASSUME macro like EXPECT but that ASSERTs instead?
#define EXPECT(expr, ...)                                                                                              \
    ({                                                                                                                 \
        auto _result_expect = (expr);                                                                                  \
        ENSURE(!_result_expect.is_error(), ##__VA_ARGS__);                                                             \
        _result_expect.disown_value();                                                                                 \
    })

#define TRY(expr)                                                                                                      \
    ({                                                                                                                 \
        auto _result_try = (expr);                                                                                     \
        if (_result_try.is_error()) {                                                                                  \
            return _result_try.error();                                                                                \
        }                                                                                                              \
        _result_try.disown_value();                                                                                    \
    })
