#pragma once

#include <ustd/Types.hh>

namespace pci {

class Bus;

} // namespace pci

namespace usb {

class Device;
struct DeviceDescriptor;

struct UsbManager {
    static void register_host_controller(const pci::Bus *bus, uint8 device, uint8 function);
    static Device *register_device(Device &&device, const DeviceDescriptor *descriptor);
};

} // namespace usb
