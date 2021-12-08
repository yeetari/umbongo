#include <core/Error.hh>
#include <core/File.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Format.hh>
#include <ustd/Log.hh>
#include <ustd/String.hh>
#include <ustd/StringBuilder.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

namespace {

usize print_device(const char *name) {
    core::File file(ustd::format("/dev/pci/{}", name).view());
    if (!file) {
        ustd::printf("lspci: no device {}\n", name);
        return 1;
    }
    PciDeviceInfo info{};
    if (auto rc = file.read({&info, sizeof(PciDeviceInfo)}); rc < 0) {
        ustd::printf("lspci: {}: {}\n", name, core::error_string(rc));
        return 1;
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
    return 0;
}

} // namespace

usize main(usize argc, const char **argv) {
    if (argc == 2) {
        return print_device(argv[1]);
    }
    ssize signed_byte_count = Syscall::invoke(Syscall::read_directory, "/dev/pci", nullptr);
    if (signed_byte_count < 0) {
        ustd::printf("lspci: {}\n", core::error_string(signed_byte_count));
        return 1;
    }
    auto byte_count = static_cast<usize>(signed_byte_count);
    ustd::LargeVector<char> data;
    data.ensure_capacity(byte_count);
    Syscall::invoke(Syscall::read_directory, "/dev/pci", data.data());
    ustd::Vector<ustd::StringView> views;
    for (usize byte_offset = 0; byte_offset != byte_count;) {
        ustd::StringView view(data.data() + byte_offset);
        views.push(view);
        byte_offset += view.length() + 1;
    }
    for (const auto &view : views) {
        if (auto rc = print_device(view.data()); rc != 0) {
            return rc;
        }
    }
    return 0;
}
