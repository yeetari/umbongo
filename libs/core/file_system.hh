#pragma once

#include <system/error.h>
#include <ustd/result.hh>
#include <ustd/string_view.hh>

namespace core {

ustd::Result<void, ub_error_t> mount(ustd::StringView target, ustd::StringView fs_type);

} // namespace core
