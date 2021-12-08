#pragma once

#include <ustd/Function.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ssize iterate_directory(ustd::StringView path, ustd::Function<void(ustd::StringView)> callback);

} // namespace core
