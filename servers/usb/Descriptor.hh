#pragma once

#include "Common.hh"

#include <ustd/Types.hh>

struct DescriptorHeader {
    uint8 length;
    DescriptorType type;
};

struct [[gnu::packed]] DeviceDescriptor : DescriptorHeader {
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

struct [[gnu::packed]] ConfigDescriptor : DescriptorHeader {
    uint16 total_length;
    uint8 interface_count;
    uint8 config_value;
    uint8 config_index;
    uint8 attributes;
    uint8 max_power;
};

struct InterfaceDescriptor : DescriptorHeader {
    uint8 interface_num;
    uint8 alt_setting;
    uint8 endpoint_count;
    uint8 iclass;
    uint8 isubclass;
    uint8 iprotocol;
    uint8 interface;
};

struct [[gnu::packed]] EndpointDescriptor : DescriptorHeader {
    uint8 address;
    uint8 attrib;
    uint16 packet_size;
    uint8 interval;
};
