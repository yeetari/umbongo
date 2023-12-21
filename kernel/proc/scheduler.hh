#pragma once

#include <ustd/unique_ptr.hh> // IWYU pragma: keep

namespace kernel {

struct RegisterState;
class Thread;

struct Scheduler {
    static void initialise();
    static void insert_thread(ustd::UniquePtr<Thread> &&thread);
    static void start();
    static void switch_next(RegisterState *);
    static void timer_handler(RegisterState *);
    static void yield(bool save_state);
    static void yield_and_kill();
};

} // namespace kernel
