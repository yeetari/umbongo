#include <core/Error.hh>
#include <core/File.hh>
#include <core/FileSystem.hh>
#include <core/Pipe.hh>
#include <core/Process.hh>
#include <ustd/Optional.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>

usize main(usize, const char **) {
    if (auto rc = core::mount("/dev", "dev"); rc < 0) {
        core::abort_error("Failed to mount /dev", rc);
    }
    if (auto rc = core::mount("/run", "ram"); rc < 0) {
        core::abort_error("Failed to mount /run", rc);
    }

    auto pipe = ustd::move(*core::create_pipe());
    if (auto rc = pipe.rebind_read(0); rc < 0) {
        core::abort_error("Failed to bind stdin pipe end", rc);
    }
    if (auto rc = core::create_process("/bin/console-server"); rc < 0) {
        core::abort_error("Failed to start /bin/console-server", rc);
    }
    if (auto rc = pipe.rebind_write(1); rc < 0) {
        core::abort_error("Failed to bind stdout pipe end", rc);
    }

    core::File keyboard;
    while (!keyboard.open("/dev/kb")) {
    }
    if (auto rc = keyboard.rebind(0); rc < 0) {
        core::abort_error("Failed to bind keyboard to stdin", rc);
    }
    if (auto rc = core::create_process("/bin/shell"); rc < 0) {
        core::abort_error("Failed to start /bin/shell", rc);
    }
    return 0;
}
