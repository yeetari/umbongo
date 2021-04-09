#pragma once

#include <ustd/Types.hh>

namespace usb {

class Device;
class HostController;

class Port {
    Device *m_device{nullptr};
    Port *m_paired{nullptr};
    volatile uint32 *m_status{nullptr};

    bool m_status_changed{false};
    uint8 m_major{0};
    uint8 m_minor{0};
    uint8 m_number{0};
    uint8 m_socket{0};

public:
    bool initialise(HostController *controller);
    bool reset();

    bool connected() const;
    bool enabled() const;
    bool initialised() const;
    uint8 speed() const;

    void clear_status_changed() { m_status_changed = false; }
    void set_status_changed() { m_status_changed = true; }

    void set_device(Device *device) { m_device = device; }
    void set_paired(Port *paired) { m_paired = paired; }
    void set_major(uint8 major) { m_major = major; }
    void set_minor(uint8 minor) { m_minor = minor; }
    void set_number(uint8 number) { m_number = number; }
    void set_socket(uint8 socket) { m_socket = socket; }

    Device *device() const { return m_device; }
    Port *paired() const { return m_paired; }
    bool status_changed() const { return m_status_changed; }
    uint8 major() const { return m_major; }
    uint8 minor() const { return m_minor; }
    uint8 number() const { return m_number; }
    uint8 socket() const { return m_socket; }
};

} // namespace usb
