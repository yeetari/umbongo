#pragma once

#include <kernel/SyscallTypes.hh>
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Vector.hh>

enum class ValueKind {
    Builtin,
    Job,
    List,
    Pipe,
    String,
};

class Value {
    const ValueKind m_kind;

protected:
    explicit Value(ValueKind kind) : m_kind(kind) {}

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;
    virtual ~Value() = default;

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    template <typename T>
    T *as_or_null();
    template <typename T>
    const T *as_or_null() const;
    template <typename T>
    bool is() const;
};

enum class BuiltinFunction {
    Cd,
};

class Builtin : public Value {
    const BuiltinFunction m_function;
    const Vector<const char *> m_args;

public:
    static constexpr auto k_kind = ValueKind::Builtin;
    Builtin(BuiltinFunction function, Vector<const char *> &&args)
        : Value(k_kind), m_function(function), m_args(ustd::move(args)) {}

    BuiltinFunction function() const { return m_function; }
    const Vector<const char *> &args() const { return m_args; }
};

class Job : public Value {
    const String m_command;
    const Vector<const char *> m_args;
    usize m_pid{0};

public:
    static constexpr auto k_kind = ValueKind::Job;
    Job(String &&command, Vector<const char *> &&args)
        : Value(k_kind), m_command(ustd::move(command)), m_args(ustd::move(args)) {}

    void await_completion() const;
    void spawn(const Vector<FdPair> &copy_fds);
};

class ListValue : public Value {
    const Vector<UniquePtr<Value>> m_values;

public:
    static constexpr auto k_kind = ValueKind::List;
    explicit ListValue(Vector<UniquePtr<Value>> &&values) : Value(k_kind), m_values(ustd::move(values)) {}

    const Vector<UniquePtr<Value>> &values() const { return m_values; }
};

class PipeValue : public Value {
    const UniquePtr<Value> m_lhs;
    const UniquePtr<Value> m_rhs;

public:
    static constexpr auto k_kind = ValueKind::Pipe;
    PipeValue(UniquePtr<Value> &&lhs, UniquePtr<Value> &&rhs)
        : Value(k_kind), m_lhs(ustd::move(lhs)), m_rhs(ustd::move(rhs)) {}

    Value &lhs() const { return *m_lhs; }
    Value &rhs() const { return *m_rhs; }
};

class StringValue : public Value {
    const String m_text;

public:
    static constexpr auto k_kind = ValueKind::String;
    explicit StringValue(String text) : Value(k_kind), m_text(ustd::move(text)) {}

    const String &text() const { return m_text; }
};

template <typename T>
T *Value::as_or_null() {
    return is<T>() ? static_cast<T *>(this) : nullptr;
}

template <typename T>
const T *Value::as_or_null() const {
    return is<T>() ? static_cast<const T *>(this) : nullptr;
}

template <typename T>
bool Value::is() const {
    return m_kind == T::k_kind;
}
