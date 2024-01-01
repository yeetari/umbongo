#include "error.hh"
#include "host_controller.hh"

#include <config/config.hh>
#include <core/directory.hh>
#include <core/error.hh>
#include <core/event_loop.hh>
#include <core/file.hh>
#include <log/log.hh>
#include <ustd/optional.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/string_builder.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>
#include <ustd/vector.hh>

namespace {

ustd::Vector<HostController> find_host_controllers(core::EventLoop &event_loop) {
    ustd::Vector<HostController> host_controllers;
    EXPECT(core::iterate_directory("/dev/pci", [&](ustd::StringView device_name) {
        auto file = EXPECT(core::File::open(ustd::format("/dev/pci/{}", device_name)));
        auto info = EXPECT(file.read<ub_pci_device_info_t>());
        if (info.clas == 0x0c && info.subc == 0x03 && info.prif == 0x30) {
            host_controllers.emplace(device_name, event_loop, ustd::move(file));
        }
    }));
    return host_controllers;
}

} // namespace

size_t main(size_t, const char **) {
    log::initialise("usb-server");
    core::EventLoop event_loop;
    config::listen(event_loop);
    config::read("usb-server");

    auto host_controllers = find_host_controllers(event_loop);
    for (auto &host_controller : host_controllers) {
        log::info("Found host controller {}", host_controller.name());
        if (auto result = host_controller.enable(); result.is_error()) {
            auto error = result.error();
            if (auto sys_error = error.as<ub_error_t>()) {
                log::error("Skipping controller {}: {}", host_controller.name(), core::error_string(*sys_error));
            } else if (auto initialisation_error = error.as<HostError>()) {
                log::error("Skipping controller {}: {}", host_controller.name(),
                           host_error_string(*initialisation_error));
            }
            continue;
        }
        event_loop.watch(host_controller.file(), UB_POLL_EVENT_WRITE);
    }
    return event_loop.run();
}
