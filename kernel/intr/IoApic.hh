#pragma once

#include <kernel/intr/InterruptType.hh>
#include <ustd/Types.hh>

class IoApic {
    volatile uint32 *const m_base{nullptr};
    const uint32 m_gsi_base{0};
    uint8 m_id{0};
    uint32 m_redirection_entry_count{0};

    template <typename T>
    T read(uint32 reg);

    template <typename T>
    void write(uint32 reg, T val);

public:
    constexpr IoApic() = default;
    IoApic(volatile uint32 *base, uint32 gsi_base);

    void set_redirection_entry(uint32 gsi, uint8 vector, InterruptPolarity polarity, InterruptTriggerMode trigger_mode);

    volatile uint32 *base() const { return m_base; }
    uint32 gsi_base() const { return m_gsi_base; }
    uint8 id() const { return m_id; }
    uint32 redirection_entry_count() const { return m_redirection_entry_count; }
};
