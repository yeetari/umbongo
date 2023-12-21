#pragma once

#include <ustd/types.hh>

namespace kernel {

enum class InterruptPolarity;
enum class InterruptTriggerMode;

class IoApic {
    volatile uint32_t *const m_base{nullptr};
    const uint32_t m_gsi_base{0};
    uint8_t m_id{0};
    uint32_t m_redirection_entry_count{0};

    template <typename T>
    T read(uint32_t reg);

    template <typename T>
    void write(uint32_t reg, T val);

public:
    constexpr IoApic() = default;
    IoApic(volatile uint32_t *base, uint32_t gsi_base);

    void set_redirection_entry(uint32_t gsi, uint8_t vector, InterruptPolarity polarity,
                               InterruptTriggerMode trigger_mode);

    volatile uint32_t *base() const { return m_base; }
    uint32_t gsi_base() const { return m_gsi_base; }
    uint8_t id() const { return m_id; }
    uint32_t redirection_entry_count() const { return m_redirection_entry_count; }
};

} // namespace kernel
