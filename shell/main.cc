#include "ast.hh"
#include "lexer.hh"
#include "line_editor.hh"
#include "parser.hh"
#include "value.hh"

#include <core/error.hh>
#include <core/key_event.hh>
#include <core/pipe.hh>
#include <core/print.hh>
#include <core/process.hh>
#include <system/syscall.hh>
#include <ustd/result.hh>
#include <ustd/string.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/vector.hh>

namespace {

void execute(Value &value, ustd::Vector<ub_fd_pair_t> &rewirings) {
    if (auto *builtin = value.as_or_null<Builtin>()) {
        const auto &args = builtin->args();
        switch (builtin->function()) {
        case BuiltinFunction::Cd:
            if (args.size() != 1 && args.size() != 2) {
                core::println("ush: cd: too many arguments");
                break;
            }
            const char *dir = args.size() == 2 ? args[1] : "/home";
            if (auto result = core::chdir(dir); result.is_error()) {
                core::println("ush: cd: {}: {}", dir, core::error_string(result.error()));
            }
            break;
        }
    } else if (auto *job = value.as_or_null<Job>()) {
        job->spawn(rewirings);
        job->await_completion();
    } else if (auto *pipe_value = value.as_or_null<PipeValue>()) {
        auto pipe = EXPECT(core::create_pipe(), "Failed to create pipe");
        rewirings.push({pipe.write_fd(), 1});
        execute(pipe_value->lhs(), rewirings);
        pipe.close_write();
        rewirings.pop();
        rewirings.push({pipe.read_fd(), 0});
        execute(pipe_value->rhs(), rewirings);
    }
}

} // namespace

void Job::await_completion() const {
    if (m_pid == 0) {
        return;
    }
    EXPECT(core::wait_pid(m_pid));
}

void Job::spawn(const ustd::Vector<ub_fd_pair_t> &copy_fds) {
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

size_t main(size_t, const char **) {
    LineEditor editor("# "sv);
    while (true) {
        editor.begin_line();
        while (true) {
            core::KeyEvent event;
            while (true) {
                auto rc = system::syscall(UB_SYS_read, 0, &event, sizeof(core::KeyEvent));
                if (rc.is_error()) {
                    return 1;
                }
                if (rc.value() > 0) {
                    break;
                }
            }
            if (auto line = editor.handle_key_event(event); !line.empty()) {
                if (line == "\n") {
                    break;
                }
                Lexer lexer(line);
                Parser parser(lexer);
                auto node = parser.parse();
                ustd::Vector<ub_fd_pair_t> rewirings;
                execute(*node->evaluate(), rewirings);
                break;
            }
        }
    }
}
