#include "Ast.hh"
#include "Lexer.hh"
#include "LineEditor.hh"
#include "Parser.hh"
#include "Value.hh"

#include <core/Error.hh>
#include <core/Process.hh>
#include <kernel/KeyEvent.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Log.hh>
#include <ustd/Optional.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

void execute(Value &value, Vector<FdPair> &rewirings) {
    if (auto *builtin = value.as_or_null<Builtin>()) {
        const auto &args = builtin->args();
        switch (builtin->function()) {
        case BuiltinFunction::Cd:
            if (args.size() != 1 && args.size() != 2) {
                logln("ush: cd: too many arguments");
                break;
            }
            const char *dir = args.size() == 2 ? args[1] : "/home";
            auto rc = Syscall::invoke(Syscall::chdir, dir);
            if (rc < 0) {
                logln("ush: cd: {}: {}", dir, core::error_string(rc));
            }
            break;
        }
    } else if (auto *job = value.as_or_null<Job>()) {
        job->spawn(rewirings);
        job->await_completion();
    } else if (auto *pipe = value.as_or_null<PipeValue>()) {
        Array<uint32, 2> pipe_fds{};
        Syscall::invoke(Syscall::create_pipe, pipe_fds.data());
        rewirings.push(FdPair{pipe_fds[1], 1});
        execute(pipe->lhs(), rewirings);
        Syscall::invoke(Syscall::close, pipe_fds[1]);
        rewirings.pop();
        rewirings.push(FdPair{pipe_fds[0], 0});
        execute(pipe->rhs(), rewirings);
        Syscall::invoke(Syscall::close, pipe_fds[0]);
    }
}

} // namespace

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    Syscall::invoke(Syscall::wait_pid, m_pid);
}

void Job::spawn(const Vector<FdPair> &copy_fds) {
    if (m_pid != 0) {
        return;
    }
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
                Vector<FdPair> rewirings;
                execute(*node->evaluate(), rewirings);
                break;
            }
        }
    }
}
