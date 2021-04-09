#pragma once

#include <kernel/pci/Device.hh>
#include <kernel/usb/Port.hh>
#include <ustd/Vector.hh>

namespace usb {

class Interrupter;
class TrbRing;
struct TransferRequestBlock;

class HostController final : public pci::Device {
    uintptr m_base{0};
    uint8 m_cap_length{0};
    uint32 m_db_offset{0};
    uint32 m_run_offset{0};

    uint8 m_context_size{0};
    void **m_context_table{nullptr};

    TrbRing *m_command_ring{nullptr};
    TrbRing *m_event_ring{nullptr};
    Interrupter *m_interrupter{nullptr};
    Vector<Port> m_ports;

    template <typename T>
    T read_cap(uint16 offset) const;
    template <typename T>
    T read_op(uint16 offset) const;

    template <typename T>
    void write_cap(uint16 offset, T value);
    template <typename T>
    void write_op(uint16 offset, T value);

public:
    HostController(const pci::Bus *bus, uint8 device, uint8 function) : pci::Device(bus, device, function) {}

    void ring_doorbell(uint8 slot, uint8 endpoint) const;

    void enable();
    void handle_interrupt();
    void on_attach(Port &port);
    void on_detach(Port &port);
    void send_command(TransferRequestBlock *command);
    void spawn_watch_thread();

    uintptr op_offset() const { return m_base + m_cap_length; }
    uint8 context_size() const { return m_context_size; }
    Vector<Port> &ports() { return m_ports; }
};

} // namespace usb
