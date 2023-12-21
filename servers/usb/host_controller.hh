#pragma once

#include "port.hh" // IWYU pragma: keep

#include <core/file.hh>
#include <system/error.h>
#include <ustd/array.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace core {

class EventLoop;

} // namespace core

class Device;
class TrbRing;
enum class DeviceError;
enum class HostError;
struct RawTrb;

struct [[gnu::packed]] CapRegs {
    uint8_t cap_length;
    size_t : 24;
    uint32_t hcs_params1;
    uint32_t hcs_params2;
    uint32_t : 32;
    uint32_t hcc_params1;
    uint32_t doorbell_offset;
    uint32_t runtime_offset;
};

struct [[gnu::packed]] OpRegs {
    uint32_t command;
    uint32_t status;
    uint32_t page_size;
    ustd::Array<uint32_t, 3> rsvd0;
    uint64_t crcr;
    ustd::Array<uint32_t, 4> rsvd1;
    uint64_t dcbaap;
    uint32_t config;
};

struct [[gnu::packed]] RunRegs {
    ustd::Array<uint32_t, 8> rsvd;
    uint32_t iman;
    uint32_t imod;
    uint32_t erst_size;
    uint32_t : 32;
    uint64_t erst_base;
    uint64_t erst_dptr;
};

struct [[gnu::packed]] DoorbellArray {
    ustd::Array<uint32_t, 256> doorbell;
};

class HostController {
    ustd::String m_name;
    core::EventLoop &m_event_loop;
    core::File m_file;
    const OpRegs *m_op_regs{nullptr};
    const RunRegs *m_run_regs{nullptr};
    const DoorbellArray *m_doorbell_array{nullptr};
    ustd::UniquePtr<TrbRing> m_command_ring;
    ustd::UniquePtr<TrbRing> m_event_ring;
    ustd::Vector<ustd::UniquePtr<Device>> m_devices;
    ustd::Vector<Port> m_ports;
    uintptr_t *m_context_table{nullptr};
    uint8_t m_context_size{0};

public:
    HostController(ustd::StringView name, core::EventLoop &event_loop, core::File &&file);
    HostController(const HostController &) = delete;
    HostController(HostController &&);
    ~HostController();

    HostController &operator=(const HostController &) = delete;
    HostController &operator=(HostController &&) = delete;

    ustd::Result<void, ustd::ErrorUnion<ub_error_t, HostError>> enable();
    void handle_interrupt();
    ustd::Result<void, ustd::ErrorUnion<DeviceError, HostError, ub_error_t>>
    handle_port_status_change(const RawTrb &event);
    void ring_doorbell(uint32_t slot, uint32_t endpoint);
    ustd::Result<RawTrb, HostError> send_command(const RawTrb &command);

    ustd::StringView name() const { return m_name; }
    core::File &file() { return m_file; }
    uint8_t context_size() const { return m_context_size; }
};
