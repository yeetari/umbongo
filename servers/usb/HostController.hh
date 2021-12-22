#pragma once

#include "Error.hh"
#include "Port.hh" // IWYU pragma: keep

#include <core/File.hh>
#include <kernel/SysError.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

class Device;
struct RawTrb;
class TrbRing;

struct [[gnu::packed]] CapRegs {
    uint8 cap_length;
    usize : 24;
    uint32 hcs_params1;
    uint32 hcs_params2;
    uint32 : 32;
    uint32 hcc_params1;
    uint32 doorbell_offset;
    uint32 runtime_offset;
};

struct [[gnu::packed]] OpRegs {
    uint32 command;
    uint32 status;
    uint32 page_size;
    ustd::Array<uint32, 3> rsvd0;
    uint64 crcr;
    ustd::Array<uint32, 4> rsvd1;
    uint64 dcbaap;
    uint32 config;
};

struct [[gnu::packed]] RunRegs {
    ustd::Array<uint32, 8> rsvd;
    uint32 iman;
    uint32 imod;
    uint32 erst_size;
    uint32 : 32;
    uint64 erst_base;
    uint64 erst_dptr;
};

struct [[gnu::packed]] DoorbellArray {
    ustd::Array<uint32, 256> doorbell;
};

class HostController {
    ustd::String m_name;
    core::File m_file;
    const OpRegs *m_op_regs{nullptr};
    const RunRegs *m_run_regs{nullptr};
    const DoorbellArray *m_doorbell_array{nullptr};
    ustd::UniquePtr<TrbRing> m_command_ring;
    ustd::UniquePtr<TrbRing> m_event_ring;
    ustd::Vector<ustd::UniquePtr<Device>> m_devices;
    ustd::Vector<Port> m_ports;
    uintptr *m_context_table{nullptr};
    uint8 m_context_size{0};

public:
    HostController(ustd::StringView name, core::File &&file);
    HostController(const HostController &) = delete;
    HostController(HostController &&) noexcept;
    ~HostController();

    HostController &operator=(const HostController &) = delete;
    HostController &operator=(HostController &&) = delete;

    ustd::Result<void, ustd::ErrorUnion<SysError, HostError>> enable();
    void handle_interrupt();
    ustd::Result<void, ustd::ErrorUnion<DeviceError, HostError, SysError>>
    handle_port_status_change(const RawTrb &event);
    void ring_doorbell(uint32 slot, uint32 endpoint);
    ustd::Result<RawTrb, HostError> send_command(const RawTrb &command);

    ustd::StringView name() const { return m_name; }
    core::File &file() { return m_file; }
    uint8 context_size() const { return m_context_size; }
};
