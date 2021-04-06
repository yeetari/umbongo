#pragma once

#include <ustd/Types.hh>

namespace usb {

enum class TransferType : uint8 {
    NoData = 0,
    WriteData = 2,
    ReadData = 3,
};

} // namespace usb
