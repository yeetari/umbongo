#pragma once

#include "Common.hh"

#include <ustd/Array.hh>

struct [[gnu::packed]] InputControlContext {
    uint32 drop;
    uint32 add;
    ustd::Array<uint32, 6> rsvd;
};

struct [[gnu::packed]] SlotContext {
    usize route_string : 20;
    usize : 5;
    bool mtt : 1;
    bool hub : 1;
    usize entry_count : 5;
    uint16 max_exit_latency;
    uint8 port_id;
    uint8 port_count;
    uint32 tt_info;
    uint8 device_address;
    usize : 19;
    SlotState slot_state : 5;
    ustd::Array<uint32, 4> rsvd;
};

struct [[gnu::packed]] EndpointContext {
    uint16 : 16;
    uint8 interval;
    usize : 9;
    usize error_count : 2;
    EndpointType endpoint_type : 3;
    usize : 10;
    uint16 max_packet_size;
    uint64 tr_dequeue_pointer;
    uint16 average_trb_length;
    ustd::Array<uint16, 7> rsvd;
};

struct LargeEndpointContext : EndpointContext {
    ustd::Array<uint8, 32> extra;
};

struct DeviceContext {
    SlotContext slot;
    ustd::Array<EndpointContext, 31> endpoints;
};

struct InputContext {
    InputControlContext control;
    DeviceContext device;
};

struct LargeDeviceContext {
    SlotContext slot;
    ustd::Array<uint8, 32> extra1;
    ustd::Array<LargeEndpointContext, 31> endpoints;
};

struct LargeInputContext {
    InputControlContext control;
    ustd::Array<uint8, 32> extra0;
    LargeDeviceContext device;
};
