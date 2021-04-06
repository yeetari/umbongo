#pragma once

#include <kernel/usb/TransferType.hh>
#include <ustd/Array.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>

namespace usb {

enum class TransferDirection : uint8 {
    Write = 0,
    Read = 1,
};

enum class TrbType : uint8 {
    Normal = 1,
    SetupStage = 2,
    DataStage = 3,
    StatusStage = 4,
    Link = 6,
    EventData = 7,
    EnableSlot = 9,
    DisableSlot = 10,
    AddressDevice = 11,
    ConfigureEndpoint = 12,
    TransferEvent = 32,
    CommandCompletionEvent = 33,
    PortStatusChangeEvent = 34,
};

struct [[gnu::packed]] TransferRequestBlock {
    union {
        uint64 data;
        void *data_ptr;
    };
    uint32 status;
    bool cycle : 1; // Cycle Bit
    bool ent : 1;   // Evaluate Next TRB
    bool isp : 1;   // Interrupt on Short Packet
    bool ns : 1;    // No Snoop
    bool chain : 1; // Chain Bit
    bool ioc : 1;   // Interrupt on Completion
    bool idt : 1;   // Immediate Data
    usize : 2;
    bool block : 1; // Block Event Interrupt or Block Set Address Request
    TrbType type : 6;
    union {
        // For Address Device and Configure Endpoint commands + Transfer and Command Completion events.
        struct {
            uint8 : 8;
            uint8 slot;
        };
        // For Data Stage and Status Stage transfers.
        struct {
            TransferDirection transfer_direction : 1;
            usize : 15;
        };
        // For Setup Stage transfer.
        struct {
            TransferType transfer_type : 2;
            usize : 14;
        };
        uint16 : 16;
    };
};

class [[gnu::aligned(64_KiB), gnu::packed]] TrbRing {
    Array<TransferRequestBlock, 256> m_queue{};
    bool m_cycle_state{true};
    usize m_index{0};

public:
    explicit TrbRing(bool insert_link);

    Span<TransferRequestBlock> dequeue();
    TransferRequestBlock *enqueue(TransferRequestBlock * trb);

    TransferRequestBlock *current() { return &m_queue[m_index]; }
};

} // namespace usb
