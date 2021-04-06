#pragma once

#include <ustd/Types.hh>

namespace usb {

enum class EndpointType : uint8 {
    IsochOut = 1,
    BulkOut = 2,
    InterruptOut = 3,
    Control = 4,
    IsochIn = 5,
    BulkIn = 6,
    InterruptIn = 7,
};

} // namespace usb
