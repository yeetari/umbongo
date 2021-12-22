#pragma once

enum class DeviceError {
    InvalidDescriptor,
    NoDriverAvailable,
    TransferTimedOut,
    UnexpectedState,
    UnsupportedSpeed,
};

enum class HostError {
    CommandTimedOut,
    HaltTimedOut,
    InvalidPortId,
    No64BitAddressing,
    ResetTimedOut,
    StartTimedOut,
    UnsupportedPageSize,
};

inline const char *device_error_string(DeviceError error) {
    switch (error) {
    case DeviceError::InvalidDescriptor:
        return "invalid descriptor";
    case DeviceError::NoDriverAvailable:
        return "no driver available";
    case DeviceError::TransferTimedOut:
        return "transfer timed out";
    case DeviceError::UnexpectedState:
        return "device entered unexpected state";
    case DeviceError::UnsupportedSpeed:
        return "unsupported speed";
    }
}

inline const char *host_error_string(HostError error) {
    switch (error) {
    case HostError::CommandTimedOut:
        return "command timed out";
    case HostError::HaltTimedOut:
        return "halt timed out";
    case HostError::InvalidPortId:
        return "invalid port id";
    case HostError::No64BitAddressing:
        return "no 64-bit addressing available";
    case HostError::ResetTimedOut:
        return "reset timed out";
    case HostError::StartTimedOut:
        return "took too long to start";
    case HostError::UnsupportedPageSize:
        return "unsupported page size";
    }
}
