#pragma once

#include <ustd/unique_ptr.hh> // IWYU pragma: keep

namespace kernel::arch {

struct RegisterState;

} // namespace kernel::arch

namespace kernel {

class Thread;

struct Scheduler {
    static void initialise();
    [[noreturn]] static void start_bsp();
    static void insert_thread(ustd::UniquePtr<Thread> &&thread);
    static void switch_next(arch::RegisterState *);
    static void timer_handler(arch::RegisterState *);
    static void yield(bool save_state);
    static void yield_and_kill();
};

} // namespace kernel
