#pragma once

#include <ustd/Types.hh>

class LocalApic {
    template <typename T>
    T read(usize reg) const;

    template <typename T>
    void write(usize reg, T val);

public:
    enum class TimerMode {
        Disabled = 0b001,
        OneShot = 0b000,
        Periodic = 0b010,
        Deadline = 0b100,
    };

    void enable();
    void send_eoi();
    void set_timer(TimerMode mode, uint8 vector);
    void set_timer_count(uint32 count);

    uint32 read_timer_count() const;
};
