#include <core/Directory.hh>
#include <core/Error.hh>
#include <core/File.hh>
#include <core/Print.hh>
#include <core/Process.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>

namespace {

void print_device(const char *name) {
    auto file_or_error = core::File::open(ustd::format("/dev/pci/{}", name));
    if (file_or_error.is_error()) {
        core::println("lspci: no device {}", name);
        core::exit(1);
    }
    auto file = file_or_error.disown_value();
    auto info = EXPECT(file.read<kernel::PciDeviceInfo>());
    ustd::StringBuilder builder;
    builder.append("{}\n", name);
    builder.append(" - Vendor ID: {:h4}\n", info.vendor_id);
    builder.append(" - Device ID: {:h4}\n", info.device_id);
    builder.append(" - Class: {:h2}\n", info.clas);
    builder.append(" - Subclass: {:h2}\n", info.subc);
    builder.append(" - Prog IF: {:h2}\n", info.prif);
    for (size_t i = 0; i < info.bars.size(); i++) {
        const auto &bar = info.bars[i];
        if (bar.address != 0) {
            builder.append(" - BAR{}: {:h} ({} bytes)\n", i, bar.address, bar.size);
        }
    }
    core::print("{}", builder.build());
}

} // namespace

size_t main(size_t argc, const char **argv) {
    if (argc == 2) {
        print_device(argv[1]);
        return 0;
    }
    auto result = core::iterate_directory("/dev/pci", [](ustd::StringView name) {
        print_device(name.data());
    });
    if (result.is_error()) {
        core::println("lscpi: {}", core::error_string(result.error()));
        return 1;
    }
    return 0;
}
