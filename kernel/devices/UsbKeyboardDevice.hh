#pragma once

#include <kernel/KeyEvent.hh>
#include <kernel/SysResult.hh>
#include <kernel/usb/Device.hh>
#include <ustd/Array.hh>
#include <ustd/RingBuffer.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>

namespace usb {

class Endpoint;

} // namespace usb

class UsbKeyboardDevice final : public usb::Device {
    Array<uint8, 8> m_buffer{};
    Array<uint8, 8> m_compare_buffer{};
    Array<bool, 8> m_modifiers{};
    RingBuffer<KeyEvent, 8> m_ring_buffer;
    usb::Endpoint *m_input_endpoint{nullptr};

    bool key_already_pressed(uint8 key) const;

public:
    explicit UsbKeyboardDevice(usb::Device &&device);

    bool can_read() override;
    bool can_write() override { return true; }
    void poll() override;
    SysResult<usize> read(Span<void> data, usize offset) override;

    const char *name() const override { return "kb"; }
};
