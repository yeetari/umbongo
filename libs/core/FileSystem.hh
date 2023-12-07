#pragma once

#include <ustd/Result.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

enum class SysError : ssize_t;

ustd::Result<void, SysError> mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
