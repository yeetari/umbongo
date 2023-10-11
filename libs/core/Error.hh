#pragma once

#include <kernel/SysError.hh> // IWYU pragma: keep
#include <ustd/StringView.hh>

namespace core {

using SysError = kernel::SysError;

[[noreturn]] void abort_error(ustd::StringView msg, SysError error);
ustd::StringView error_string(SysError error);

} // namespace core
