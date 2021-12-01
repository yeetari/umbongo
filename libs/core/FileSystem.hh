#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ssize mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
