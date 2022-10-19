#include <core/Debug.hh>
#include <core/Error.hh>
#include <core/Process.hh>
#include <ipc/Channel.hh>
#include <ipc/Encoder.hh>
#include <ipc/Endpoint.hh>
#include <kernel/api/BootFs.hh>
#include <system/Syscall.hh>
#include <system/Types.h>
#include <ustd/Assert.hh>
#include <ustd/Format.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace {

enum class MessageKind {
    LookupService,
    RegisterService,
};

struct Service {
    ustd::String name;
    ipc::Channel channel;
};

ustd::Result<ustd::Tuple<BootFsEntry *, core::Handle>, ub_error_t> map_boot_fs() {
    auto vmo_handle = core::wrap_handle(0);
    auto size = TRY(system::syscall<size_t>(SYS_vmo_size, *vmo_handle));

    BootFsEntry *boot_fs = nullptr;
    auto vmr_handle = TRY(system::syscall<ub_handle_t>(SYS_vmr_allocate, size, &boot_fs));
    TRY(system::syscall<uint8_t *>(SYS_vmr_map, vmr_handle, *vmo_handle));
    return ustd::make_tuple(boot_fs, core::wrap_handle(vmr_handle));
}

} // namespace

size_t main() {
    auto [boot_fs, boot_fs_handle] = EXPECT(map_boot_fs());
    core::debug_line("Mapped BootFS at {}", boot_fs);

    auto endpoint = EXPECT(ipc::create_endpoint());

    const auto &config = EXPECT(boot_fs->find("/etc/root-server.cfg"sv));
    ustd::StringView config_view(config.data().as<const char>());
    for (size_t line_start = 0, i = 0; i < config_view.length(); i++) {
        if (config_view[i] != '\n') {
            continue;
        }
        auto path = config_view.substr(ustd::exchange(line_start, i + 1), i);
        auto entry = boot_fs->find(path);
        if (!entry) {
            core::debug_line("Failed to find {}", path);
            continue;
        }
        if (auto result = core::spawn_process(entry->data(), path); result.is_error()) {
            auto error_string = core::error_string(result.error());
            core::debug_line("Failed to spawn {}: {}", path, static_cast<const void *>(error_string.data()));
        }
    }

    ustd::Vector<Service> service_map;
    auto find_service = [&](ustd::StringView name) -> ustd::Optional<const Service &> {
        for (const auto &service : service_map) {
            if (service.name == name) {
                return service;
            }
        }
        return {};
    };

    while (true) {
        auto [decoder, handles] = EXPECT(endpoint.recv());
        switch (EXPECT(decoder.decode<MessageKind>())) {
        case MessageKind::LookupService: {
            const auto name = EXPECT(decoder.decode<ustd::StringView>());
            auto service = find_service(name);

            break;
        }
        case MessageKind::RegisterService: {
            const auto name = EXPECT(decoder.decode<ustd::StringView>());
            ENSURE(!find_service(name));

            auto channel = EXPECT(ipc::create_channel(handles.first()));
            service_map.push({ustd::String(name), ustd::move(channel)});
            core::debug_line("Registered service {}", name);
            break;
        }
        }
    }
}
