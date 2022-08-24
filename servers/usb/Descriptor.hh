#pragma once

#include "Common.hh"

#include <ustd/Types.hh>

struct DescriptorHeader {
    uint8_t length;
    DescriptorType type;
};

struct [[gnu::packed]] DeviceDescriptor : DescriptorHeader {
    uint16_t version;
    uint8_t dclass;
    uint8_t dsubclass;
    uint8_t dprotocol;
    uint8_t packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t release;
    uint8_t manufacturer_index;
    uint8_t product_index;
    uint8_t serial_index;
    uint8_t config_count;
};

struct [[gnu::packed]] ConfigDescriptor : DescriptorHeader {
    uint16_t total_length;
    uint8_t interface_count;
    uint8_t config_value;
    uint8_t config_index;
    uint8_t attributes;
    uint8_t max_power;
};

struct InterfaceDescriptor : DescriptorHeader {
    uint8_t interface_num;
    uint8_t alt_setting;
    uint8_t endpoint_count;
    uint8_t iclass;
    uint8_t isubclass;
    uint8_t iprotocol;
    uint8_t interface;
};

struct [[gnu::packed]] EndpointDescriptor : DescriptorHeader {
    uint8_t address;
    uint8_t attrib;
    uint16_t packet_size;
    uint8_t interval;
};
