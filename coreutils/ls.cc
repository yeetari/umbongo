#include <core/Error.hh>
#include <kernel/Syscall.hh>
#include <ustd/Algorithm.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

bool compare_name(ustd::StringView lhs, ustd::StringView rhs) {
    if (int rc = memcmp(lhs.data(), rhs.data(), ustd::min(lhs.length(), rhs.length()))) {
        return rc > 0;
    }
    return lhs.length() > rhs.length();
}

usize main(usize argc, const char **argv) {
    const char *path = argc == 1 ? "." : argv[1];
    ssize rc = Syscall::invoke(Syscall::read_directory, path, nullptr);
    if (rc < 0) {
        printf("ls: {}: {}\n", path, core::error_string(rc));
        return 1;
    }
    auto byte_count = static_cast<usize>(rc);
    ustd::LargeVector<char> data;
    data.ensure_capacity(byte_count);
    Syscall::invoke(Syscall::read_directory, path, data.data());
    ustd::Vector<ustd::StringView> views;
    for (usize byte_offset = 0; byte_offset != byte_count;) {
        ustd::StringView view(data.data() + byte_offset);
        views.push(view);
        byte_offset += view.length() + 1;
    }
    ustd::sort(views, compare_name);
    for (const auto &view : views) {
        printf("{}\n", view);
    }
    return 0;
}
