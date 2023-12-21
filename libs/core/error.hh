#pragma once

#include <system/error.h>
#include <ustd/string_view.hh>

namespace core {

[[noreturn]] void abort_error(ustd::StringView msg, ub_error_t error);
ustd::StringView error_string(ub_error_t error);

} // namespace core
