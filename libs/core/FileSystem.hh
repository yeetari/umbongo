#pragma once

#include <system/Error.h>
#include <ustd/Result.hh>
#include <ustd/StringView.hh>

namespace core {

ustd::Result<void, ub_error_t> mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
