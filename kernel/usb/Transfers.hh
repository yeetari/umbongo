#pragma once

#include <ustd/Types.hh>

namespace usb {

enum class ControlRecipient : uint8 {
    Device = 0,
    Interface = 1,
    Endpoint = 2,
};

enum class ControlType : uint8 {
    Standard = 0,
    Class = 1,
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
    ControlType type : 2;
    ControlDirection direction : 1;
    ControlRequest request;
    uint16 value;
    uint16 index;
    uint16 length;
};

} // namespace usb
