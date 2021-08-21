#include <kernel/devices/Device.hh>

#include <kernel/ScopedLock.hh> // IWYU pragma: keep
#include <kernel/SpinLock.hh>
#include <kernel/devices/DevFs.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

Vector<Device *> s_devices;
SpinLock s_devices_lock;

} // namespace

Vector<Device *> Device::all_devices() {
    ScopedLock locker(s_devices_lock);
    return s_devices;
}

Device::Device() {
    ScopedLock locker(s_devices_lock);
    s_devices.push(this);
}

Device::~Device() {
    ScopedLock locker(s_devices_lock);
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
