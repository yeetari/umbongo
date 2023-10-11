#pragma once

#include <core/Error.hh>
#include <ustd/Result.hh>
#include <ustd/Span.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh> // IWYU pragma: keep

enum class TransferType : uint8_t;

enum class TrbType : uint8_t {
    // Transfers.
    Normal = 1,
    SetupStage = 2,
    DataStage = 3,
    StatusStage = 4,

    Link = 6,
    EventData = 7,

    // Commands.
    EnableSlotCmd = 9,
    DisableSlotCmd = 10,
    AddressDeviceCmd = 11,
    ConfigureEndpointCmd = 12,
    NoOpCmd = 23,

    // Events.
    TransferEvent = 32,
    CommandCompletionEvent = 33,
    PortStatusChangeEvent = 34,
};

struct [[gnu::packed]] RawTrb {
    uint64_t data;
    uint32_t status;
    bool cycle : 1;
    bool evaluate_next : 1;
    bool : 1;
    bool : 1;
    bool chain : 1;
    bool event_on_completion : 1;
    bool immediate_data : 1;
    size_t : 2;
    bool block : 1; // Block Event Interrupt or Block Set Address Request
    TrbType type : 6;
    union {
        // For AddressDeviceCmd, DisableSlotCmd, CommandCompletionEvent and TransferEvent.
        struct [[gnu::packed]] {
            size_t endpoint_id : 5; // Only for TransferEvent
            size_t : 3;
            uint8_t slot_id;
        };
        // For SetupStage control transfer.
        struct {
            TransferType transfer_type : 2;
            size_t : 14;
        };
        // For DataStage and StatusStage control transfers.
        struct {
            bool read : 1;
            size_t : 15;
        };
    };
};

class TrbRing {
    RawTrb *const m_ring;
    size_t m_index{0};
    bool m_cycle_state{true};

public:
    static ustd::Result<ustd::UniquePtr<TrbRing>, core::SysError> create(bool insert_link);

    TrbRing(RawTrb *ring, bool insert_link);
    // TODO: Free m_ring in destructor.

    ustd::Span<RawTrb> dequeue();
    RawTrb &enqueue(const RawTrb &trb);
    ustd::Result<uintptr_t, core::SysError> physical_base() const;
    ustd::Result<uintptr_t, core::SysError> physical_head() const;
    RawTrb &operator[](size_t index) const;
};
