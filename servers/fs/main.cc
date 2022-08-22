#include "Drive.hh"
#include "Ufs.hh"
#include "UfsTypes.hh"

#include <core/EventLoop.hh>
#include <core/File.hh>
#include <core/Time.hh>
#include <log/Log.hh>
#include <ustd/Array.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/Vector.hh>

usize main(usize, const char **) {
    log::initialise("fs-server");
    core::File drive_file;
    while (true) {
        auto drive_file_or_error = core::File::open("/dev/sda");
        if (!drive_file_or_error.is_error()) {
            drive_file = drive_file_or_error.disown_value();
            break;
        }
        core::sleep(50000000ul);
    }

    Drive drive(ustd::move(drive_file));
    core::EventLoop event_loop;
    Ufs ufs(drive);
    EXPECT(ufs.initialise(event_loop));
    return event_loop.run();
}
