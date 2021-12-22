#pragma once

#include <core/Error.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>

namespace core {

ustd::Result<void, SysError> mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
