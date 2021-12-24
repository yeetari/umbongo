#include "Ast.hh"
#include "Lexer.hh"
#include "LineEditor.hh"
#include "Parser.hh"
#include "Value.hh"

#include <core/Error.hh>
#include <core/KeyEvent.hh>
#include <core/Print.hh>
#include <core/Process.hh>
#include <core/Syscall.hh>
#include <ustd/Array.hh>
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace {

void execute(Value &value, ustd::Vector<kernel::FdPair> &rewirings) {
    if (auto *builtin = value.as_or_null<Builtin>()) {
        const auto &args = builtin->args();
        switch (builtin->function()) {
        case BuiltinFunction::Cd:
            if (args.size() != 1 && args.size() != 2) {
                core::println("ush: cd: too many arguments");
                break;
            }
            const char *dir = args.size() == 2 ? args[1] : "/home";
            if (auto result = core::syscall(Syscall::chdir, dir); result.is_error()) {
                core::println("ush: cd: {}: {}", dir, core::error_string(result.error()));
            }
            break;
        }
    } else if (auto *job = value.as_or_null<Job>()) {
        job->spawn(rewirings);
        job->await_completion();
    } else if (auto *pipe = value.as_or_null<PipeValue>()) {
        // TODO: Use core::Pipe
        ustd::Array<uint32, 2> pipe_fds{};
        EXPECT(core::syscall(Syscall::create_pipe, pipe_fds.data()));
        rewirings.push(kernel::FdPair{pipe_fds[1], 1});
        execute(pipe->lhs(), rewirings);
        EXPECT(core::syscall(Syscall::close, pipe_fds[1]));
        rewirings.pop();
        rewirings.push(kernel::FdPair{pipe_fds[0], 0});
        execute(pipe->rhs(), rewirings);
        EXPECT(core::syscall(Syscall::close, pipe_fds[0]));
    }
}

} // namespace

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    EXPECT(core::syscall(Syscall::wait_pid, m_pid));
}

void Job::spawn(const ustd::Vector<kernel::FdPair> &copy_fds) {
    if (m_pid != 0) {
        return;
    }
    auto result = core::create_process(m_command.data(), m_args, copy_fds);
    if (result.is_error()) {
        core::println("ush: {}: command not found", m_command);
        return;
    }
    m_pid = result.value();
}

usize main(usize, const char **) {
    LineEditor editor("# "sv);
    while (true) {
        editor.begin_line();
        while (true) {
            core::KeyEvent event;
            while (true) {
                auto rc = core::syscall(Syscall::read, 0, &event, sizeof(core::KeyEvent));
                if (rc.is_error()) {
                    return 1;
                }
                if (rc.value() > 0) {
                    break;
                }
            }
            if (auto line = editor.handle_key_event(event); !line.empty()) {
                Lexer lexer(line);
                Parser parser(lexer);
                auto node = parser.parse();
                ustd::Vector<kernel::FdPair> rewirings;
                execute(*node->evaluate(), rewirings);
                break;
            }
        }
    }
}
