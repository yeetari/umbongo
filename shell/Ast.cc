#include "Ast.hh"
#include "Value.hh"

#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace ast {
namespace {

void build_string_vector(const Value &value, Vector<const char *> &vector) {
    if (const auto *list = value.as_or_null<ListValue>()) {
        for (const auto &list_value : list->values()) {
            build_string_vector(*list_value, vector);
        }
    } else if (const auto *string = value.as_or_null<StringValue>()) {
        vector.push(string->text().data());
    } else {
        ENSURE_NOT_REACHED();
    }
}

Vector<const char *> build_string_vector(const Value &value) {
    Vector<const char *> vector;
    build_string_vector(value, vector);
    return vector;
}

String find_command(StringView name) {
    String path(name.length() + 1);
    memcpy(path.data() + 1, name.data(), name.length());
    path.data()[0] = '/';
    return path;
}

} // namespace

void Node::dump(usize indent) const {
    constexpr usize indent_space_count = 2;
    for (usize i = 0; i < indent * indent_space_count; i++) {
        dbg_put_char(' ');
    }
}

void Command::dump(usize indent) const {
    Node::dump(indent);
    dbgln("Command");
    m_node->dump(indent + 1);
}

void List::dump(usize indent) const {
    Node::dump(indent);
    dbgln("List");
    for (const auto &node : m_nodes) {
        node->dump(indent + 1);
    }
}

void Pipe::dump(usize indent) const {
    Node::dump(indent);
    dbgln("Pipe");
    m_lhs->dump(indent + 1);
    m_rhs->dump(indent + 1);
}

void StringLiteral::dump(usize indent) const {
    Node::dump(indent);
    dbgln("StringLiteral({})", m_text.view());
}

UniquePtr<Value> Command::evaluate() const {
    auto args = build_string_vector(*m_node->evaluate());
    auto command = find_command(args[0]);
    return ustd::make_unique<Job>(ustd::move(command), ustd::move(args));
}

UniquePtr<Value> List::evaluate() const {
    Vector<UniquePtr<Value>> values;
    values.ensure_capacity(m_nodes.size());
    for (const auto &node : m_nodes) {
        values.push(node->evaluate());
    }
    return ustd::make_unique<ListValue>(ustd::move(values));
}

UniquePtr<Value> Pipe::evaluate() const {
    Array<uint32, 2> pipe_fds{};
    Syscall::invoke(Syscall::create_pipe, pipe_fds.data());
    if (auto lhs = m_lhs->evaluate(); auto *job = lhs->as_or_null<Job>()) {
        Vector<FdPair> copy_fds;
        copy_fds.push(FdPair{pipe_fds[1], 1});
        job->spawn(copy_fds);
        job->await_completion();
    }
    auto rhs = m_rhs->evaluate();
    if (auto *job = rhs->as_or_null<Job>()) {
        Vector<FdPair> copy_fds;
        copy_fds.push(FdPair{pipe_fds[0], 0});
        job->spawn(copy_fds);
    }
    Syscall::invoke(Syscall::close, pipe_fds[0]);
    Syscall::invoke(Syscall::close, pipe_fds[1]);
    return rhs;
}

UniquePtr<Value> StringLiteral::evaluate() const {
    return ustd::make_unique<StringValue>(m_text);
}

} // namespace ast
