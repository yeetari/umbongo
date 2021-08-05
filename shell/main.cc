#include "Ast.hh"
#include "Lexer.hh"
#include "Parser.hh"
#include "Value.hh"

#include <core/Process.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

String read_line() {
    // TODO: Make Vector with inline stack capacity.
    Vector<char> buffer;
    uint32 cursor_pos = 0;
    while (true) {
        // NOLINTNEXTLINE
        Array<char, 16> buf;
        usize bytes_read = 0;
        while (bytes_read == 0) {
            ssize rc = Syscall::invoke(Syscall::read, 0, buf.data(), buf.size());
            ENSURE(rc >= 0);
            bytes_read = static_cast<usize>(rc);
        }
        for (usize i = 0; i < bytes_read; i++) {
            char ch = buf[i];
            if (ch == '\b') {
                if (cursor_pos == 0) {
                    continue;
                }
                log_put_char(ch);
                buffer.remove(cursor_pos - 1);
                cursor_pos--;
                continue;
            }
            log_put_char(ch);
            if (ch == '\n') {
                return String(buffer.data(), buffer.size());
            }
            buffer.push(ch);
            cursor_pos++;
        }
    }
}

} // namespace

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    while (Syscall::invoke<bool>(Syscall::is_alive, m_pid)) {
    }
}

void Job::spawn(const Vector<FdPair> &copy_fds) {
    ssize rc = core::create_process(m_command.data(), m_args, copy_fds);
    if (rc < 0) {
        logln("ush: {}: command not found", m_command.view());
        return;
    }
    m_pid = static_cast<usize>(rc);
}

usize main(usize, const char **) {
    while (true) {
        log("$ ");
        String line = read_line();
        if (line.empty()) {
            continue;
        }
        Lexer lexer(ustd::move(line));
        Parser parser(lexer);
        auto node = parser.parse();
        if (auto value = node->evaluate(); auto *job = value->as_or_null<Job>()) {
            job->spawn({});
            job->await_completion();
        }
    }
}
