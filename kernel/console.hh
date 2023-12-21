#pragma once

struct BootInfo;

namespace kernel {

struct Console {
    static void initialise(BootInfo *boot_info);
    static void put_char(char ch);
};

} // namespace kernel
