#pragma once

#include <ustd/array.hh>
#include <ustd/types.hh>

enum class EndpointType : uint8_t;
enum class SlotState : uint8_t;

struct [[gnu::packed]] InputControlContext {
    uint32_t drop;
    uint32_t add;
    ustd::Array<uint32_t, 6> rsvd;
};

struct [[gnu::packed]] SlotContext {
    size_t route_string : 20;
    size_t : 5;
    bool mtt : 1;
    bool hub : 1;
    size_t entry_count : 5;
    uint16_t max_exit_latency;
    uint8_t port_id;
    uint8_t port_count;
    uint32_t tt_info;
    uint8_t device_address;
    size_t : 19;
    SlotState slot_state : 5;
    ustd::Array<uint32_t, 4> rsvd;
};

struct [[gnu::packed]] EndpointContext {
    uint16_t : 16;
    uint8_t interval;
    size_t : 9;
    size_t error_count : 2;
    EndpointType endpoint_type : 3;
    size_t : 10;
    uint16_t max_packet_size;
    uint64_t tr_dequeue_pointer;
    uint16_t average_trb_length;
    ustd::Array<uint16_t, 7> rsvd;
};

struct LargeEndpointContext : EndpointContext {
    ustd::Array<uint8_t, 32> extra;
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
    ustd::Array<uint8_t, 32> extra1;
    ustd::Array<LargeEndpointContext, 31> endpoints;
};

struct LargeInputContext {
    InputControlContext control;
    ustd::Array<uint8_t, 32> extra0;
    LargeDeviceContext device;
};
