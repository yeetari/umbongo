#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

ssize mount(StringView target, StringView fs_type);

} // namespace core
