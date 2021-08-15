#pragma once

#include <ustd/Types.hh>

namespace acpi {

class HpetTable;

} // namespace acpi

struct RegisterState;
class Thread;

struct Scheduler {
    static void initialise(acpi::HpetTable *hpet_table);
    static void insert_thread(Thread *thread);
    static void start();
    static void switch_next(RegisterState *);
    static void timer_handler(RegisterState *);
    static void wait(usize millis);
    static void yield();
    static void yield_and_kill();
};
