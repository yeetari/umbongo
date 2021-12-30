#include <core/Directory.hh>
#include <core/Error.hh>
#include <core/Print.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Numeric.hh>
#include <ustd/Result.hh>
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
    auto result = core::iterate_directory(path, [&](ustd::StringView name) {
        names.emplace(name);
    });
    if (result.is_error()) {
        core::println("ls: {}: {}", path, core::error_string(result.error()));
        return 1;
    }
    ustd::sort(names, compare_name);
    for (const auto &name : names) {
        core::println("{}", name);
    }
    return 0;
}
