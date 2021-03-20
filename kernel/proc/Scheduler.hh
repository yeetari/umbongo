#pragma once

#include <ustd/Types.hh>

class LocalApic;
class Process;
struct RegisterState;

struct Scheduler {
    static void initialise(LocalApic *apic);
    static void insert_process(Process *process);
    static void start();
    static void switch_next(RegisterState *);
};
