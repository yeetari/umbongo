#pragma once

#include <kernel/usb/Device.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace usb {

class KeyboardDevice final : public Device {
    Array<uint8, 8> m_buffer{};
    Array<uint8, 8> m_compare_buffer{};
    Array<bool, 8> m_modifiers{};
    Endpoint *m_input_endpoint{nullptr};

    bool key_already_pressed(uint8 key) const;

public:
    explicit KeyboardDevice(Device &&device);

    void poll() override;
};

} // namespace usb
