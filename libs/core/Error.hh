#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

[[noreturn]] void abort_error(ustd::StringView msg, ssize rc);
ustd::StringView error_string(ssize rc);

} // namespace core
