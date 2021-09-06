#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

[[noreturn]] void abort_error(StringView msg, ssize rc);
StringView error_string(ssize rc);

} // namespace core
