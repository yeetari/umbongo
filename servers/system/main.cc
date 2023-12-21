#include <core/error.hh>
#include <core/file.hh>
#include <core/file_system.hh>
#include <core/pipe.hh>
#include <core/process.hh>
#include <ustd/result.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

size_t main(size_t, const char **) {
    EXPECT(core::mount("/run", "ram"), "Failed to mount /run");
    EXPECT(core::create_process("/bin/log-server"), "Failed to start /bin/log-server");
    EXPECT(core::create_process("/bin/config-server"), "Failed to start /bin/config-server");
    auto pipe = EXPECT(core::create_pipe(), "Failed to create stdio pipe");
    EXPECT(pipe.rebind_read(0), "Failed to bind stdin pipe end");
    EXPECT(core::create_process("/bin/console-server"), "Failed to start /bin/console-server");
    EXPECT(pipe.rebind_write(1), "Failed to bind stdout pipe end");
    EXPECT(core::create_process("/bin/usb-server"), "Failed to start /bin/usb-server");

    core::File keyboard;
    while (true) {
        auto keyboard_or_error = core::File::open("/dev/kb");
        if (!keyboard_or_error.is_error()) {
            keyboard = keyboard_or_error.disown_value();
            break;
        }
    }
    if (auto result = keyboard.rebind(0); result.is_error()) {
        core::abort_error("Failed to bind keyboard to stdin", result.error());
    }
    EXPECT(core::create_process("/bin/shell"), "Failed to start /bin/shell");
    return 0;
}
