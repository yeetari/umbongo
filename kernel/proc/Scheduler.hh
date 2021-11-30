#pragma once

#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

struct RegisterState;
class Thread;

struct Scheduler {
    static void initialise();
    static void insert_thread(UniquePtr<Thread> &&thread);
    static void setup();
    static void start();
    static void switch_next(RegisterState *);
    static void timer_handler(RegisterState *);
    static void wait(usize millis);
    static void yield(bool save_state);
    static void yield_and_kill();
};
