#include <kernel/devices/Device.hh>

#include <kernel/devices/DevFs.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

Vector<Device *> s_devices;

} // namespace

const Vector<Device *> &Device::all_devices() {
    return s_devices;
}

Device::Device() {
    s_devices.push(this);
}

Device::~Device() {
    for (uint32 i = 0; i < s_devices.size(); i++) {
        if (s_devices[i] == this) {
            s_devices.remove(i);
            break;
        }
    }
}

void Device::disconnect() {
    m_connected = false;
    DevFs::notify_detach(this);
}
