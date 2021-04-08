#include <kernel/usb/UsbManager.hh>

#include <kernel/cpu/Processor.hh>
#include <kernel/pci/Bus.hh>
#include <kernel/usb/Descriptors.hh>
#include <kernel/usb/Device.hh>
#include <kernel/usb/HostController.hh>
#include <kernel/usb/hid/KeyboardDevice.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace usb {
namespace {

Vector<HostController> s_controllers;
uint8 s_vector_base = 70;

void irq_handler(RegisterState *) {
    for (auto &controller : s_controllers) {
        controller.handle_interrupt();
    }
}

} // namespace

void UsbManager::register_host_controller(const pci::Bus *bus, uint8 device, uint8 function) {
    logln(" usb: Registered xHCI controller at {:h4}:{:h2}:{:h2}:{:h2}", bus->segment_num(), bus->num(), device,
          function);
    auto &controller = s_controllers.emplace(bus, device, function);
    controller.configure();
    if (controller.msix_supported()) {
        controller.enable_msix(s_vector_base);
    } else if (controller.msi_supported()) {
        controller.enable_msi(s_vector_base);
    } else {
        logln(" usb: Controller doesn't support MSI or MSI-X, skipping!");
        return;
    }
    Processor::wire_interrupt(s_vector_base++, &irq_handler);
    controller.enable();
}

Device *UsbManager::register_device(Device &&device, const DeviceDescriptor *descriptor) {
    if (descriptor->dclass != 0) {
        logln(" usb: Found device {:h2}:{:h2}:{:h2}", descriptor->dclass, descriptor->dsubclass, descriptor->dprotocol);
        return nullptr;
    }

    Device *ret = nullptr;
    device.read_configuration();
    device.walk_configuration([&](void *desc, DescriptorType type) {
        if (type == DescriptorType::Interface) {
            auto *interface = static_cast<InterfaceDescriptor *>(desc);
            logln(" usb: Found interface {:h2}:{:h2}:{:h2}", interface->iclass, interface->isubclass,
                  interface->iprotocol);
            if (interface->iclass != 3) {
                return;
            }
            logln(" usb: Found HID device {:h2}:{:h2}", interface->isubclass, interface->iprotocol);
            if (interface->isubclass == 1 && interface->iprotocol == 1) {
                logln(" usb: Found HID keyboard");
                ret = new KeyboardDevice(ustd::move(device));
            }
        }
    });
    return ret;
}

} // namespace usb
