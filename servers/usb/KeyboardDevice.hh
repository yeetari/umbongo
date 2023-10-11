#pragma once

#include "Device.hh"

#include <core/Pipe.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

namespace core {

class EventLoop;
class Timer;

} // namespace core

enum class DeviceError;

class KeyboardDevice final : public Device {
    uint8_t *m_dma_buffer{nullptr};
    core::Pipe m_pipe;
    ustd::Array<uint8_t, 8> m_cmp_buffer{};
    ustd::Array<bool, 8> m_modifiers{};
    ustd::UniquePtr<core::Timer> m_repeat_timer;
    size_t m_repeat_delay{400};
    uint8_t m_last_code{0};
    size_t m_last_time{0};

    bool key_already_pressed(uint8_t key) const;

public:
    explicit KeyboardDevice(Device &&device);
    KeyboardDevice(const KeyboardDevice &) = delete;
    KeyboardDevice(KeyboardDevice &&) = delete;
    ~KeyboardDevice() override;

    KeyboardDevice &operator=(const KeyboardDevice &) = delete;
    KeyboardDevice &operator=(KeyboardDevice &&) = delete;

    ustd::Result<void, DeviceError> enable(core::EventLoop &event_loop);
    void poll() override;
};
