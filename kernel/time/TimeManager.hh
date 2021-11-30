#pragma once

#include <ustd/Types.hh>

namespace acpi {

class HpetTable;

} // namespace acpi

struct TimeManager {
    static void initialise(acpi::HpetTable *hpet_table);
    static void spin(uint64 millis);
    static void update();
    static uint64 ns_since_boot();
};
