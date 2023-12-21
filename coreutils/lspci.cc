#include <core/directory.hh>
#include <core/error.hh>
#include <core/file.hh>
#include <core/print.hh>
#include <core/process.hh>
#include <system/system.h>
#include <ustd/array.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/string_builder.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

namespace {

void print_device(const char *name) {
    auto file_or_error = core::File::open(ustd::format("/dev/pci/{}", name));
    if (file_or_error.is_error()) {
        core::println("lspci: no device {}", name);
        core::exit(1);
    }
    auto file = file_or_error.disown_value();
    auto info = EXPECT(file.read<ub_pci_device_info_t>());
    ustd::StringBuilder builder;
    builder.append("{}\n", name);
    builder.append(" - Vendor ID: {:h4}\n", info.vendor_id);
    builder.append(" - Device ID: {:h4}\n", info.device_id);
    builder.append(" - Class: {:h2}\n", info.clas);
    builder.append(" - Subclass: {:h2}\n", info.subc);
    builder.append(" - Prog IF: {:h2}\n", info.prif);
    for (size_t i = 0; i < 6; i++) {
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
