#pragma once

#include <system/error.h>
#include <ustd/result.hh>
#include <ustd/span.hh> // IWYU pragma: keep
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>

class Device;
class TrbRing;
enum class DeviceError;
enum class EndpointType : uint8_t;
enum class HostError;
enum class TransferType : uint8_t;

enum class ControlRecipient : uint8_t {
    Device = 0,
    Interface = 1,
    Endpoint = 2,
};

enum class ControlDirection : uint8_t {
    HostToDevice = 0,
    DeviceToHost = 1,
};

enum class ControlRequest : uint8_t {
    GetDescriptor = 6,
    SetConfiguration = 9,
    SetInterface = 11,
};

struct [[gnu::packed]] ControlTransfer {
    ControlRecipient recipient : 5;
    size_t : 2;
    ControlDirection direction : 1;
    ControlRequest request;
    uint16_t value;
    uint16_t index;
    uint16_t length;
};

class Endpoint {
    const Device &m_device;
    const uint32_t m_id;
    ustd::UniquePtr<TrbRing> m_transfer_ring;

public:
    Endpoint(const Device &device, uint32_t id);
    Endpoint(const Endpoint &) = delete;
    Endpoint(Endpoint &&) = delete;
    ~Endpoint();

    Endpoint &operator=(const Endpoint &) = delete;
    Endpoint &operator=(Endpoint &&) = delete;

    ustd::Result<void, ustd::ErrorUnion<HostError, ub_error_t>> setup(EndpointType type, uint16_t packet_size);
    ustd::Result<void, DeviceError> send_control(ControlTransfer transfer, TransferType transfer_type,
                                                 ustd::Span<void> data = {});
    void set_interval(uint8_t interval);

    uint32_t id() const { return m_id; }
    TrbRing &transfer_ring() { return *m_transfer_ring; }
};
