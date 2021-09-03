#pragma once

#include <ustd/Types.hh>

namespace console {

struct TerminalSize {
    uint32 column_count;
    uint32 row_count;
};

TerminalSize terminal_size();

} // namespace console
