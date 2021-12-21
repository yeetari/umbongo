#include <core/Directory.hh>
#include <core/Error.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

bool compare_name(ustd::String &lhs, ustd::String &rhs) {
    if (int rc = __builtin_memcmp(lhs.data(), rhs.data(), ustd::min(lhs.length(), rhs.length()))) {
        return rc > 0;
    }
    return lhs.length() > rhs.length();
}

usize main(usize argc, const char **argv) {
    const char *path = argc == 1 ? "." : argv[1];
    ustd::Vector<ustd::String> names;
    auto rc = core::iterate_directory(path, [&](ustd::StringView name) {
        names.emplace(name);
    });
    if (rc < 0) {
        ustd::printf("ls: {}: {}\n", path, core::error_string(rc));
        return 1;
    }
    ustd::sort(names, compare_name);
    for (const auto &name : names) {
        ustd::printf("{}\n", name.view());
    }
    return 0;
}
