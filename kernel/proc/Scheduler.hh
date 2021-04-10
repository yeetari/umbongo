#pragma once

#include <ustd/Types.hh>

namespace acpi {

class HpetTable;

} // namespace acpi

class Process;
struct RegisterState;

struct Scheduler {
    static void initialise(acpi::HpetTable *hpet_table);
    static void insert_process(Process *process);
    static void start();
    static void switch_next(RegisterState *);
};
