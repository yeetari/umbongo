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
#include <ustd/Result.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

namespace {

void execute(Value &value, ustd::Vector<FdPair> &rewirings) {
    if (auto *builtin = value.as_or_null<Builtin>()) {
        const auto &args = builtin->args();
        switch (builtin->function()) {
        case BuiltinFunction::Cd:
            if (args.size() != 1 && args.size() != 2) {
                ustd::printf("ush: cd: too many arguments\n");
                break;
            }
            const char *dir = args.size() == 2 ? args[1] : "/home";
            if (auto result = Syscall::invoke(Syscall::chdir, dir); result.is_error()) {
                ustd::printf("ush: cd: {}: {}\n", dir, core::error_string(result.error()));
            }
            break;
        }
    } else if (auto *job = value.as_or_null<Job>()) {
        job->spawn(rewirings);
        job->await_completion();
    } else if (auto *pipe = value.as_or_null<PipeValue>()) {
        // TODO: Use core::Pipe
        ustd::Array<uint32, 2> pipe_fds{};
        EXPECT(Syscall::invoke(Syscall::create_pipe, pipe_fds.data()));
        rewirings.push(FdPair{pipe_fds[1], 1});
        execute(pipe->lhs(), rewirings);
        EXPECT(Syscall::invoke(Syscall::close, pipe_fds[1]));
        rewirings.pop();
        rewirings.push(FdPair{pipe_fds[0], 0});
        execute(pipe->rhs(), rewirings);
        EXPECT(Syscall::invoke(Syscall::close, pipe_fds[0]));
    }
}

} // namespace

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    EXPECT(Syscall::invoke(Syscall::wait_pid, m_pid));
}

void Job::spawn(const ustd::Vector<FdPair> &copy_fds) {
    if (m_pid != 0) {
        return;
    }
    auto result = core::create_process(m_command.data(), m_args, copy_fds);
    if (result.is_error()) {
        printf("ush: {}: command not found\n", m_command.view());
        return;
    }
    m_pid = result.value();
}

usize main(usize, const char **) {
    LineEditor editor("# "sv);
    while (true) {
        editor.begin_line();
        while (true) {
            KeyEvent event;
            while (true) {
                auto rc = Syscall::invoke(Syscall::read, 0, &event, sizeof(KeyEvent));
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
                ustd::Vector<FdPair> rewirings;
                execute(*node->evaluate(), rewirings);
                break;
            }
        }
    }
}
