#pragma once

#include <ustd/Types.hh>

namespace kernel {

enum class MessageType {
    Fixed = 0b000,
    LowestPriority = 0b001,
    Smi = 0b010,
    Nmi = 0b100,
    Init = 0b101,
    Startup = 0b110,
    External = 0b111,
};

enum class DestinationMode {
    Physical = 0,
    Logical = 1,
};

enum class Level {
    Deassert = 0,
    Assert = 1,
};

enum class TriggerMode {
    Edge = 0,
    Level = 1,
};

enum class DestinationShorthand {
    Destination = 0b00,
    Self = 0b01,
    AllIncludingSelf = 0b10,
    AllExcludingSelf = 0b11,
};

class LocalApic {
    template <typename T>
    T read(size_t reg) const;

    template <typename T>
    void write(size_t reg, T val);

public:
    enum class TimerMode {
        Disabled = 0b001,
        OneShot = 0b000,
        Periodic = 0b010,
        Deadline = 0b100,
    };

    void enable();
    void send_eoi();
    void send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                  DestinationShorthand shorthand, uint8_t destination);
    void send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                  DestinationShorthand shorthand);
    void send_ipi(uint8_t vector, MessageType type, DestinationMode mode, Level level, TriggerMode trigger_mode,
                  uint8_t destination);
    void set_timer(TimerMode mode, uint8_t vector);
    void set_timer_count(uint32_t count);

    uint32_t read_timer_count() const;
};

} // namespace kernel
