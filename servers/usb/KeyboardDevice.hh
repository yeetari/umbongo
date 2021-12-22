#pragma once

#include "Device.hh"
#include "Error.hh"

#include <core/Pipe.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

class KeyboardDevice final : public Device {
    uint8 *m_dma_buffer{nullptr};
    core::Pipe m_pipe;
    ustd::Array<uint8, 8> m_cmp_buffer{};
    ustd::Array<bool, 8> m_modifiers{};

    bool key_already_pressed(uint8 key) const;

public:
    explicit KeyboardDevice(Device &&device) : Device(ustd::move(device)) {}

    ustd::Result<void, DeviceError> enable();
    void poll() override;
};
