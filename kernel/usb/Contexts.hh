#pragma once

#include <kernel/usb/EndpointType.hh>
#include <kernel/usb/SlotState.hh>
#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace usb {

struct [[gnu::packed]] InputControlContext {
    uint32 disable;
    uint32 enable;
    Array<uint32, 5> controller_reserved;
    uint8 config_value;
    uint8 interface_num;
    uint8 alternate_setting;
    uint8 : 8;
};

struct [[gnu::packed]] SlotContext {
    usize route_string : 20;
    usize speed : 4;
    usize : 1;
    bool mtt : 1;
    bool hub : 1;
    usize entry_count : 5;
    uint16 max_exit_latency;
    uint8 port_num;
    uint8 port_count;
    uint8 tt_hub_slot;
    uint8 tt_port_num;
    usize tt_think_time : 2;
    usize : 4;
    usize interrupter_target : 10;
    uint8 device_address;
    usize : 19;
    SlotState slot_slate : 5;
    Array<uint32, 4> controller_reserved;
};

struct [[gnu::packed]] EndpointContext {
    usize ep_state : 3;
    usize : 5;
    usize mult : 2;
    usize max_primary_streams : 5;
    bool lsa : 1;
    uint8 interval;
    uint8 max_esit_high;
    usize : 1;
    usize error_count : 2;
    EndpointType ep_type : 3;
    usize : 1;
    bool host_initiate_disable : 1;
    uint8 max_burst_size;
    uint16 max_packet_size;
    uint64 tr_dequeue_ptr;
    uint16 average_trb_length;
    uint16 max_esit_low;
    Array<uint32, 3> controller_reserved;
};

struct [[gnu::aligned(4_KiB)]] DeviceContext {
    InputControlContext input;
    SlotContext slot;
    Array<EndpointContext, 31> endpoints;
};

} // namespace usb
