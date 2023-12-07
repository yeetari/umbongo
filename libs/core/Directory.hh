#pragma once

#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
// IWYU pragma: no_forward_declare ustd::Function

namespace core {

enum class SysError : ssize_t;

ustd::Result<size_t, SysError> iterate_directory(ustd::StringView path,
                                                 ustd::Function<void(ustd::StringView)> callback);

} // namespace core
