#pragma once

#include <system/Types.h>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/StringView.hh>

namespace core {

ustd::Result<void, ub_error_t> spawn_process(ustd::Span<const uint8_t> binary, ustd::StringView name);

} // namespace core
