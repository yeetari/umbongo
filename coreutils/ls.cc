#include <core/directory.hh>
#include <core/error.hh>
#include <core/print.hh>
#include <ustd/algorithm.hh>
#include <ustd/numeric.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

bool compare_name(ustd::String &lhs, ustd::String &rhs) {
    if (int rc = __builtin_memcmp(lhs.data(), rhs.data(), ustd::min(lhs.length(), rhs.length()))) {
        return rc > 0;
    }
    return lhs.length() > rhs.length();
}

size_t main(size_t argc, const char **argv) {
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
