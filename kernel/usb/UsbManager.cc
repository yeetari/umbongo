#include <kernel/usb/UsbManager.hh>

#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/devices/UsbKeyboardDevice.hh>
#include <kernel/pci/Bus.hh>
#include <kernel/usb/Descriptors.hh>
#include <kernel/usb/Device.hh>
#include <kernel/usb/HostController.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace usb {
namespace {

constexpr uint8 k_base_vector = 70;

ustd::Vector<HostController> s_controllers;
uint8 s_current_vector = k_base_vector;

void irq_handler(RegisterState *regs) {
    s_controllers[static_cast<uint32>(regs->int_num) - k_base_vector].handle_interrupt();
}

} // namespace

void UsbManager::register_host_controller(const pci::Bus *bus, uint8 device, uint8 function) {
    ustd::dbgln(" usb: Registered xHCI controller at {:h4}:{:h2}:{:h2}:{:h2}", bus->segment_num(), bus->num(), device,
                function);
    auto &controller = s_controllers.emplace(bus, device, function);
    controller.configure();
    if (controller.msix_supported()) {
        controller.enable_msix(s_current_vector);
    } else if (controller.msi_supported()) {
        controller.enable_msi(s_current_vector);
    } else {
        ustd::dbgln(" usb: Controller doesn't support MSI or MSI-X, skipping!");
        return;
    }
    Processor::wire_interrupt(s_current_vector++, &irq_handler);
    controller.enable();
}

void UsbManager::spawn_watch_threads() {
    for (auto &controller : s_controllers) {
        controller.spawn_watch_thread();
    }
}

Device *UsbManager::register_device(Device &&device, const DeviceDescriptor *descriptor) {
    if (descriptor->dclass != 0) {
        ustd::dbgln(" usb: Found device {:h2}:{:h2}:{:h2}", descriptor->dclass, descriptor->dsubclass,
                    descriptor->dprotocol);
        return nullptr;
    }

    Device *ret = nullptr;
    device.read_configuration();
    device.walk_configuration([&](void *desc, DescriptorType type) {
        if (type == DescriptorType::Interface) {
            auto *interface = static_cast<InterfaceDescriptor *>(desc);
            ustd::dbgln(" usb: Found interface {:h2}:{:h2}:{:h2}", interface->iclass, interface->isubclass,
                        interface->iprotocol);
            if (interface->iclass != 3) {
                return;
            }
            ustd::dbgln(" usb: Found HID device {:h2}:{:h2}", interface->isubclass, interface->iprotocol);
            if (interface->isubclass == 1 && interface->iprotocol == 1) {
                ustd::dbgln(" usb: Found HID keyboard");
                ret = new UsbKeyboardDevice(ustd::move(device));
            }
        }
    });
    return ret;
}

} // namespace usb
