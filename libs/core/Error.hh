#pragma once

#include <system/Error.h>
#include <ustd/StringView.hh>

namespace core {

[[noreturn]] void abort_error(ustd::StringView msg, ub_error_t error);
ustd::StringView error_string(ub_error_t error);

} // namespace core
