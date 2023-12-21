#include <kernel/dev/device.hh>

#include <kernel/dev/dev_fs.hh>
#include <ustd/string.hh>
#include <ustd/utility.hh>

namespace kernel {

Device::Device(ustd::String &&path) : m_path(ustd::move(path)) {
    DevFs::notify_attach(this, m_path);
}

void Device::disconnect() {
    m_connected = false;
    DevFs::notify_detach(this);
}

} // namespace kernel
