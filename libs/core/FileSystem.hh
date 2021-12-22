#pragma once

#include <kernel/SysError.hh>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>

namespace core {

ustd::Result<void, SysError> mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
