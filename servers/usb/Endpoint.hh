#pragma once

#include "Common.hh"
#include "Error.hh"

#include <kernel/SysError.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh> // IWYU pragma: keep
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>

class Device;
class TrbRing;

enum class ControlRecipient : uint8 {
    Device = 0,
    Interface = 1,
    Endpoint = 2,
};

enum class ControlDirection : uint8 {
    HostToDevice = 0,
    DeviceToHost = 1,
};

enum class ControlRequest : uint8 {
    GetDescriptor = 6,
    SetConfiguration = 9,
    SetInterface = 11,
};

struct [[gnu::packed]] ControlTransfer {
    ControlRecipient recipient : 5;
    usize : 2;
    ControlDirection direction : 1;
    ControlRequest request;
    uint16 value;
    uint16 index;
    uint16 length;
};

class Endpoint {
    const Device &m_device;
    const uint32 m_id;
    ustd::UniquePtr<TrbRing> m_transfer_ring;

public:
    Endpoint(const Device &device, uint32 id);
    Endpoint(const Endpoint &) = delete;
    Endpoint(Endpoint &&) = delete;
    ~Endpoint();

    Endpoint &operator=(const Endpoint &) = delete;
    Endpoint &operator=(Endpoint &&) = delete;

    ustd::Result<void, ustd::ErrorUnion<HostError, SysError>> setup(EndpointType type, uint16 packet_size);
    ustd::Result<void, DeviceError> send_control(ControlTransfer transfer, TransferType transfer_type,
                                                 ustd::Span<void> data = {});
    void set_interval(uint8 interval);

    uint32 id() const { return m_id; }
    TrbRing &transfer_ring() { return *m_transfer_ring; }
};
