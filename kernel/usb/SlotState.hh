#pragma once

#include <ustd/Types.hh>

namespace usb {

enum class SlotState : uint8 {
    DisableEnabled = 0,
    Default = 1,
    Addressed = 2,
    Configured = 3,
};

} // namespace usb
