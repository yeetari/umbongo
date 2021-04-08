#pragma once

class Process;
struct RegisterState;

struct Scheduler {
    static void initialise();
    static void insert_process(Process *process);
    static void start();
    static void switch_next(RegisterState *);
};
