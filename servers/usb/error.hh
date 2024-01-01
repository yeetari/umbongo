#pragma once

#include <ustd/string_view.hh>

enum class DeviceError {
    InvalidDescriptor,
    NoDriverAvailable,
    TransferTimedOut,
    UnexpectedState,
    UnsupportedSpeed,
};

enum class HostError {
    BadAlignment,
    CommandTimedOut,
    HaltTimedOut,
    InvalidPortId,
    No64BitAddressing,
    ResetTimedOut,
    StartTimedOut,
    UnsupportedPageSize,
};

inline ustd::StringView device_error_string(DeviceError error) {
    switch (error) {
    case DeviceError::InvalidDescriptor:
        return "invalid descriptor"sv;
    case DeviceError::NoDriverAvailable:
        return "no driver available"sv;
    case DeviceError::TransferTimedOut:
        return "transfer timed out"sv;
    case DeviceError::UnexpectedState:
        return "device entered unexpected state"sv;
    case DeviceError::UnsupportedSpeed:
        return "unsupported speed"sv;
    }
}

inline ustd::StringView host_error_string(HostError error) {
    switch (error) {
    case HostError::BadAlignment:
        return "bad alignment"sv;
    case HostError::CommandTimedOut:
        return "command timed out"sv;
    case HostError::HaltTimedOut:
        return "halt timed out"sv;
    case HostError::InvalidPortId:
        return "invalid port id"sv;
    case HostError::No64BitAddressing:
        return "no 64-bit addressing available"sv;
    case HostError::ResetTimedOut:
        return "reset timed out"sv;
    case HostError::StartTimedOut:
        return "took too long to start"sv;
    case HostError::UnsupportedPageSize:
        return "unsupported page size"sv;
    }
}
