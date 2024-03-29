#pragma once

#include <ustd/types.hh>

namespace console {

struct TerminalSize {
    uint32_t column_count;
    uint32_t row_count;
};

TerminalSize terminal_size();

} // namespace console
