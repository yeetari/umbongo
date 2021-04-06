#pragma once

#include <ustd/Types.hh>

namespace usb {

enum class DescriptorType : uint8 {
    Device = 1,
    Configuration = 2,
    Interface = 4,
    Endpoint = 5,
};

struct DescriptorHeader {
    uint8 length;
    DescriptorType type;
};

struct [[gnu::packed]] DeviceDescriptor : public DescriptorHeader {
    uint16 version;
    uint8 dclass;
    uint8 dsubclass;
    uint8 dprotocol;
    uint8 packet_size;
    uint16 vendor_id;
    uint16 product_id;
    uint16 release;
    uint8 manufacturer_index;
    uint8 product_index;
    uint8 serial_index;
    uint8 config_count;
};

struct [[gnu::packed]] ConfigDescriptor : public DescriptorHeader {
    uint16 total_length;
    uint8 interface_count;
    uint8 config_value;
    uint8 config_index;
    uint8 attributes;
    uint8 max_power;
};

struct InterfaceDescriptor : public DescriptorHeader {
    uint8 interface_num;
    uint8 alt_setting;
    uint8 endpoint_count;
    uint8 iclass;
    uint8 isubclass;
    uint8 iprotocol;
    uint8 interface;
};

struct [[gnu::packed]] EndpointDescriptor : public DescriptorHeader {
    uint8 address;
    uint8 attrib;
    uint16 packet_size;
    uint8 interval;
};

} // namespace usb
