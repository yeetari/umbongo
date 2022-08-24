#pragma once

#include <core/Error.hh>
#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ustd::Result<size_t, SysError> iterate_directory(ustd::StringView path,
                                                 ustd::Function<void(ustd::StringView)> callback);

} // namespace core
