#pragma once

#include "Device.hh"

#include <core/File.hh>

enum class ScsiError {
    CommandFailed,
    PhaseError,
    ShortRead,
    Unknown,
};

class StorageDevice final : public Device {
    core::File m_file;
    Endpoint *m_input_endpoint{nullptr};
    Endpoint *m_output_endpoint{nullptr};

    ustd::Result<void, ustd::ErrorUnion<DeviceError, ScsiError>> read_sector(ustd::Span<uint8> data, uint32 lba);

public:
    explicit StorageDevice(Device &&device) : Device(ustd::move(device)) {}

    ustd::Result<void, DeviceError> enable(core::EventLoop &event_loop) override;
};
