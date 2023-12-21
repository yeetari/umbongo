#pragma once

#include <system/error.h>
#include <ustd/function.hh> // IWYU pragma: keep
#include <ustd/result.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
// IWYU pragma: no_forward_declare ustd::Function

namespace core {

ustd::Result<size_t, ub_error_t> iterate_directory(ustd::StringView path,
                                                   ustd::Function<void(ustd::StringView)> callback);

} // namespace core
