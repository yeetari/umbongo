#pragma once

struct BootInfo;

struct Console {
    static void initialise(BootInfo *boot_info);
    static void put_char(char ch);
};
