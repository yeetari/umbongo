#pragma once

#include <kernel/api/Types.hh> // IWYU pragma: keep
#include <ustd/String.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>
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
    const ustd::Vector<const char *> m_args;

public:
    static constexpr auto k_kind = ValueKind::Builtin;
    Builtin(BuiltinFunction function, ustd::Vector<const char *> &&args)
        : Value(k_kind), m_function(function), m_args(ustd::move(args)) {}

    BuiltinFunction function() const { return m_function; }
    const ustd::Vector<const char *> &args() const { return m_args; }
};

class Job : public Value {
    const ustd::String m_command;
    const ustd::Vector<const char *> m_args;
    size_t m_pid{0};

public:
    static constexpr auto k_kind = ValueKind::Job;
    Job(ustd::String &&command, ustd::Vector<const char *> &&args)
        : Value(k_kind), m_command(ustd::move(command)), m_args(ustd::move(args)) {}

    void await_completion() const;
    void spawn(const ustd::Vector<kernel::FdPair> &copy_fds);
};

class ListValue : public Value {
    const ustd::Vector<ustd::UniquePtr<Value>> m_values;

public:
    static constexpr auto k_kind = ValueKind::List;
    explicit ListValue(ustd::Vector<ustd::UniquePtr<Value>> &&values) : Value(k_kind), m_values(ustd::move(values)) {}

    const ustd::Vector<ustd::UniquePtr<Value>> &values() const { return m_values; }
};

class PipeValue : public Value {
    const ustd::UniquePtr<Value> m_lhs;
    const ustd::UniquePtr<Value> m_rhs;

public:
    static constexpr auto k_kind = ValueKind::Pipe;
    PipeValue(ustd::UniquePtr<Value> &&lhs, ustd::UniquePtr<Value> &&rhs)
        : Value(k_kind), m_lhs(ustd::move(lhs)), m_rhs(ustd::move(rhs)) {}

    Value &lhs() const { return *m_lhs; }
    Value &rhs() const { return *m_rhs; }
};

class StringValue : public Value {
    const ustd::String m_text;

public:
    static constexpr auto k_kind = ValueKind::String;
    explicit StringValue(ustd::String text) : Value(k_kind), m_text(ustd::move(text)) {}

    const ustd::String &text() const { return m_text; }
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
