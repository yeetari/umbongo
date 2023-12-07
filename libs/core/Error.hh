#pragma once

#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace core {

enum class SysError : ssize_t {
#define E(_, name, value) name = (value),
#include <kernel/api/Errors.in>
#undef E
};

[[noreturn]] void abort_error(ustd::StringView msg, SysError error);
ustd::StringView error_string(SysError error);

} // namespace core
