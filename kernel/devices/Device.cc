#include <kernel/devices/Device.hh>

#include <kernel/devices/DevFs.hh>
#include <ustd/String.hh>
#include <ustd/Utility.hh>

namespace kernel {

Device::Device(ustd::String &&path) : m_path(ustd::move(path)) {
    DevFs::notify_attach(this, m_path.view());
}

void Device::disconnect() {
    m_connected = false;
    DevFs::notify_detach(this);
}

} // namespace kernel
