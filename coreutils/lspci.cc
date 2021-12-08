#include <core/Directory.hh>
#include <core/Error.hh>
#include <core/File.hh>
#include <core/Process.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Format.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace {

void print_device(const char *name) {
    core::File file(ustd::format("/dev/pci/{}", name).view());
    if (!file) {
        ustd::printf("lspci: no device {}\n", name);
        core::exit(1);
    }
    PciDeviceInfo info{};
    if (auto rc = file.read({&info, sizeof(PciDeviceInfo)}); rc < 0) {
        ustd::printf("lspci: {}: {}\n", name, core::error_string(rc));
        core::exit(1);
    }
    ustd::StringBuilder builder;
    builder.append("{}\n", name);
    builder.append(" - Vendor ID: {:h4}\n", info.vendor_id);
    builder.append(" - Device ID: {:h4}\n", info.device_id);
    for (usize i = 0; i < info.bars.size(); i++) {
        if (info.bars[i] != 0) {
            builder.append(" - BAR{}: {:h}\n", i, info.bars[i]);
        }
    }
    ustd::printf("{}", builder.build().view());
}

} // namespace

usize main(usize argc, const char **argv) {
    if (argc == 2) {
        print_device(argv[1]);
    }
    auto rc = core::iterate_directory("/dev/pci", [](ustd::StringView name) {
        print_device(name.data());
    });
    if (rc < 0) {
        ustd::printf("lscpi: {}\n", core::error_string(rc));
        return 1;
    }
    return 0;
}
