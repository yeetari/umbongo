#pragma once

#include <ustd/Types.hh>

namespace kernel::acpi {

class HpetTable;

} // namespace kernel::acpi

namespace kernel {

struct TimeManager {
    static void initialise(acpi::HpetTable *hpet_table);
    static void spin(uint64_t millis);
    static void update();
    static uint64_t ns_since_boot();
};

} // namespace kernel
