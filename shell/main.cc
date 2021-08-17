#include "Ast.hh"
#include "Lexer.hh"
#include "LineEditor.hh"
#include "Parser.hh"
#include "Value.hh"

#include <core/Process.hh>
#include <kernel/KeyEvent.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Log.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    Syscall::invoke(Syscall::wait_pid, m_pid);
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
    LineEditor editor("# "sv);
    while (true) {
        editor.begin_line();
        while (true) {
            KeyEvent event;
            while (true) {
                ssize rc = Syscall::invoke(Syscall::read, 0, &event, sizeof(KeyEvent));
                if (rc < 0) {
                    return 1;
                }
                if (rc > 0) {
                    break;
                }
            }
            if (auto line = editor.handle_key_event(event)) {
                if (line->empty()) {
                    break;
                }
                Lexer lexer(ustd::move(*line));
                Parser parser(lexer);
                auto node = parser.parse();
                if (auto value = node->evaluate(); auto *job = value->as_or_null<Job>()) {
                    job->spawn({});
                    job->await_completion();
                }
                break;
            }
        }
    }
}
