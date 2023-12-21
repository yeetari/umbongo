#include <config/config.hh>
#include <core/print.hh>
#include <ustd/algorithm.hh>
#include <ustd/optional.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/vector.hh>

size_t main(size_t argc, const char **argv) {
    if ((argc != 1 && argc != 2) || (argc == 2 && ustd::StringView(argv[1]) == "--help")) {
        core::println("Usage: {} [key][=value]", argv[0]);
        return 0;
    }
    if (argc == 1) {
        for (const auto &value : config::list_all()) {
            core::println("{}", value);
        }
        return 0;
    }
    ustd::StringView arg(argv[1]);
    if (auto dot_index = ustd::index_of(arg, '.')) {
        auto domain = arg.substr(0, *dot_index);
        if (auto equal_index = ustd::index_of(arg, '=')) {
            auto key = arg.substr(*dot_index + 1, *equal_index);
            auto value = arg.substr(*equal_index + 1);
            if (!config::update(domain, key, value)) {
                core::println("syscfg: failed to update config for {}: no key named {}", domain, key);
            }
        } else {
            auto key = arg.substr(*dot_index + 1);
            if (auto value = config::lookup(domain, key)) {
                core::println("{}.{}={}", domain, key, *value);
            } else {
                core::println("syscfg: no key named {} in config for {}", key, domain);
            }
        }
    } else {
        core::println("syscfg: {}: not a valid key", argv[1]);
    }
    return 0;
}
