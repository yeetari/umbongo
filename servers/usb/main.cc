#include "Error.hh"
#include "HostController.hh"

#include <core/Directory.hh>
#include <core/Error.hh>
#include <core/EventLoop.hh>
#include <core/File.hh>
#include <kernel/SyscallTypes.hh>
#include <log/Log.hh>
#include <ustd/Format.hh>
#include <ustd/Optional.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

ustd::Vector<HostController> find_host_controllers(core::EventLoop &event_loop) {
    ustd::Vector<HostController> host_controllers;
    EXPECT(core::iterate_directory("/dev/pci", [&](ustd::StringView device_name) {
        auto file = EXPECT(core::File::open(ustd::format("/dev/pci/{}", device_name)));
        auto info = EXPECT(file.read<kernel::PciDeviceInfo>());
        if (info.clas == 0x0c && info.subc == 0x03 && info.prif == 0x30) {
            host_controllers.emplace(device_name, event_loop, ustd::move(file));
        }
    }));
    return host_controllers;
}

} // namespace

usize main(usize, const char **) {
    log::initialise("usb-server");
    core::EventLoop event_loop;
    auto host_controllers = find_host_controllers(event_loop);
    for (auto &host_controller : host_controllers) {
        if (auto result = host_controller.enable(); result.is_error()) {
            auto error = result.error();
            if (auto sys_error = error.as<core::SysError>()) {
                log::warn("Skipping controller {}: {}", host_controller.name(), core::error_string(*sys_error));
            } else if (auto initialisation_error = error.as<HostError>()) {
                log::warn("Skipping controller {}: {}", host_controller.name(),
                          host_error_string(*initialisation_error));
            }
            continue;
        }
        event_loop.watch(host_controller.file(), kernel::PollEvents::Write);
    }
    return event_loop.run();
}
