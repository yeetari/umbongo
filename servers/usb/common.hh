#pragma once

#include <ustd/types.hh>

enum class DescriptorType : uint8_t {
    Device = 1,
    Configuration = 2,
    Interface = 4,
    Endpoint = 5,
};

enum class EndpointType : uint8_t {
    IsochOut = 1,
    BulkOut = 2,
    InterruptOut = 3,
    Control = 4,
    IsochIn = 5,
    BulkIn = 6,
    InterruptIn = 7,
};

enum class SlotState : uint8_t {
    DisabledEnabled = 0,
    Default = 1,
    Addressed = 2,
    Configured = 3,
};

enum class TransferType : uint8_t {
    NoData = 0,
    WriteData = 2,
    ReadData = 3,
};
