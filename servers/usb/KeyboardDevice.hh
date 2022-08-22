#pragma once

#include "Device.hh"
#include "Error.hh"

#include <core/Pipe.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

namespace core {

class EventLoop;
class Timer;

} // namespace core

class KeyboardDevice final : public Device {
    uint8 *m_dma_buffer{nullptr};
    core::Pipe m_pipe;
    ustd::Array<uint8, 8> m_cmp_buffer{};
    ustd::Array<bool, 8> m_modifiers{};
    ustd::UniquePtr<core::Timer> m_repeat_timer;
    usize m_repeat_delay{400};
    uint8 m_last_code{0};
    usize m_last_time{0};

    bool key_already_pressed(uint8 key) const;

public:
    explicit KeyboardDevice(Device &&device);
    KeyboardDevice(const KeyboardDevice &) = delete;
    KeyboardDevice(KeyboardDevice &&) = delete;
    ~KeyboardDevice() override;

    KeyboardDevice &operator=(const KeyboardDevice &) = delete;
    KeyboardDevice &operator=(KeyboardDevice &&) = delete;

    ustd::Result<void, DeviceError> enable(core::EventLoop &event_loop) override;
    void poll() override;
};
