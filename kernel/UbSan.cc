#include <ustd/Array.hh>
#include <ustd/Log.hh>
#include <ustd/Numeric.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace {

struct SourceLocation {
    const char *file_name;
    uint32 line;
    uint32 column;
};

enum class TypeKind : uint16 {
    Integer = 0,
    Float = 1,
    Unknown = 0xffff,
};

class TypeDescriptor {
    TypeKind m_kind;
    uint16 m_info;
    ustd::Array<char, 1> m_name;

public:
    // NOLINTNEXTLINE
    uint32 bit_width() const {
        if (m_kind == TypeKind::Integer) {
            // NOLINTNEXTLINE
            return 1u << (m_info >> 1u);
        }
        if (m_kind == TypeKind::Float) {
            return m_info;
        }
        return ustd::Limits<uint32>::max();
    }
    bool is_signed() const { return (m_info & 1u) == 1u; }
    TypeKind kind() const { return m_kind; }
    ustd::StringView name() const { return m_name.data(); }
};

using ValueHandle = uintptr;

class Value {
    const TypeDescriptor &m_type;
    ValueHandle m_handle;

public:
    Value(const TypeDescriptor &type, ValueHandle handle) : m_type(type), m_handle(handle) {}

    bool is_inline() const { return m_type.bit_width() <= sizeof(ValueHandle) * 8; }

    const TypeDescriptor &type() const { return m_type; }
    ValueHandle handle() const { return m_handle; }
};

struct AlignmentAssumptionData {
    SourceLocation location;
    SourceLocation assumption_location;
    const TypeDescriptor &type;
};

struct ImplicitConversionData {
    SourceLocation location;
    const TypeDescriptor &src_type;
    const TypeDescriptor &dst_type;
};

struct InvalidBuiltinData {
    SourceLocation location;
    uint8 kind;
};

struct InvalidValueData {
    SourceLocation location;
    const TypeDescriptor &type;
};

struct NonNullReturnData {
    SourceLocation location;
};

struct OverflowData {
    SourceLocation location;
    const TypeDescriptor &type;
};

struct PointerOverflowData {
    SourceLocation location;
};

struct ShiftOutOfBoundsData {
    SourceLocation location;
    const TypeDescriptor &lhs_type;
    const TypeDescriptor &rhs_type;
};

struct TypeMismatchData {
    SourceLocation location;
    const TypeDescriptor &type;
    uint8 log_alignment;
    uint8 type_check_kind;
};

struct UnreachableData {
    SourceLocation location;
};

void die() {
    asm volatile("cli; hlt");
}

void handle_overflow(OverflowData *data, ValueHandle lhs, ValueHandle rhs, const char *op) {
    dbgln("{}: error: {} integer overflow: {} {} {} cannot be represented in type {}", data->location,
          data->type.is_signed() ? "signed" : "unsigned", Value(data->type, lhs), op, Value(data->type, rhs),
          data->type.name());
    die();
}

} // namespace

extern "C" void __ubsan_handle_add_overflow(OverflowData *data, ValueHandle lhs, ValueHandle rhs) {
    handle_overflow(data, lhs, rhs, "+");
}

extern "C" void __ubsan_handle_alignment_assumption(AlignmentAssumptionData *data, ValueHandle, ValueHandle alignment,
                                                    ValueHandle offset) {
    if (offset == 0) {
        ustd::dbgln("{}: error: assumption of {} byte alignment for pointer of type {} failed", data->location,
                    alignment, data->type.name());
    } else {
        ustd::dbgln("{}: error: assumption of {} byte alignment (with offset of {} byte) for pointer of type {} failed",
                    data->location, alignment, offset, data->type.name());
    }
    if (data->assumption_location.file_name != nullptr) {
        ustd::dbgln("{}: note: alignment assumption was specified here", data->assumption_location);
    }
    die();
}

extern "C" void __ubsan_handle_builtin_unreachable(UnreachableData *data) {
    ustd::dbgln("{}: error: execution reached a program point marked as unreachable", data->location);
    die();
}

extern "C" void __ubsan_handle_divrem_overflow(OverflowData *data, ValueHandle, ValueHandle) {
    // TODO: Handle signed integer overflow case.
    ustd::dbgln("{}: error: divison by zero", data->location);
    die();
}

extern "C" void __ubsan_handle_implicit_conversion(ImplicitConversionData *data, ValueHandle src, ValueHandle dst) {
    ustd::dbgln(
        "{}: error: implicit conversion from type {} of value {} ({}-bit, {}signed) to type {} changed the value to "
        "{} ({}-bit, {}signed}",
        data->location, data->src_type.name(), Value(data->src_type, src), data->src_type.bit_width(),
        data->src_type.is_signed() ? "" : "un", data->dst_type.name(), Value(data->dst_type, dst),
        data->dst_type.bit_width(), data->dst_type.is_signed() ? "" : "un");
    die();
}

extern "C" void __ubsan_handle_invalid_builtin(InvalidBuiltinData *data) {
    ustd::StringView kind = data->kind == 1 ? "clz"sv : "ctz"sv;
    ustd::dbgln("{}: error: passing zero to {}, which is invalid", data->location, kind);
    die();
}

extern "C" void __ubsan_handle_load_invalid_value(InvalidValueData *data, ValueHandle handle) {
    ustd::dbgln("{}: error: load of value {} which is not valid for type {}", data->location, Value(data->type, handle),
                data->type.name());
    die();
}

extern "C" void __ubsan_handle_missing_return(UnreachableData *data) {
    ustd::dbgln("{}: error: execution reached the end of a value-returning function without returning a value",
                data->location);
    die();
}

extern "C" void __ubsan_handle_mul_overflow(OverflowData *data, ValueHandle lhs, ValueHandle rhs) {
    handle_overflow(data, lhs, rhs, "*");
}

extern "C" void __ubsan_handle_nonnull_return_v1(NonNullReturnData *data, SourceLocation *) {
    ustd::dbgln("{}: error: null pointer returned from function declared to never return null", data->location);
    die();
}

extern "C" void __ubsan_handle_pointer_overflow(PointerOverflowData *data, ValueHandle base, ValueHandle result) {
    ustd::dbgln("{}: error: pointer index expression with base {} overflowed to {}", data->location,
                reinterpret_cast<void *>(base), reinterpret_cast<void *>(result));
    die();
}

extern "C" void __ubsan_handle_shift_out_of_bounds(ShiftOutOfBoundsData *data, ValueHandle lhs_handle,
                                                   ValueHandle rhs_handle) {
    // TODO: Handle negative RHS and non-inline case.
    Value lhs(data->lhs_type, lhs_handle);
    Value rhs(data->rhs_type, rhs_handle);
    if (rhs.is_inline() && !rhs.type().is_signed() && rhs.handle() >= lhs.type().bit_width()) {
        ustd::dbgln("{}: error: shift amount {} is too large for {}-bit type {}", data->location, rhs,
                    lhs.type().bit_width(), lhs.type().name());
    } else {
        // TODO: Handle negative LHS.
        ustd::dbgln("{}: error: left shift of {} by {} places cannot be represented in type {}", data->location, lhs,
                    rhs, lhs.type().name());
    }
    die();
}

extern "C" void __ubsan_handle_sub_overflow(OverflowData *data, ValueHandle lhs, ValueHandle rhs) {
    handle_overflow(data, lhs, rhs, "-");
}

extern "C" void __ubsan_handle_type_mismatch_v1(TypeMismatchData *data, ValueHandle pointer) {
    uintptr alignment = static_cast<uintptr>(1u) << data->log_alignment;
    constexpr ustd::Array kinds{
        "load of"sv,
        "store to"sv,
        "reference binding to"sv,
        "member access within"sv,
        "member call on"sv,
        "constructor call on"sv,
        "downcast of"sv,
        "downcast of"sv,
        "upcast of"sv,
        "call to virtual base of"sv,
        "_Nonnull binding to"sv,
        "dynamic operation on"sv,
    };
    if (pointer == 0) {
        ustd::dbgln("{}: error: {} null pointer of type {}", data->location, kinds[data->type_check_kind],
                    data->type.name());
    } else if ((pointer & (alignment - 1)) != 0) {
        ustd::dbgln("{}: error: {} misaligned address {} for type {}, which requires {} byte alignment", data->location,
                    kinds[data->type_check_kind], reinterpret_cast<void *>(pointer), data->type.name(), alignment);
    } else {
        ustd::dbgln("{}: error: {} address {} with insufficient space for an object of type {}", data->location,
                    kinds[data->type_check_kind], reinterpret_cast<void *>(pointer), data->type.name());
    }
    die();
}

namespace ustd::detail {

template <>
inline void log_single(LogFn fn, const char *, SourceLocation location) {
    const char *file_name = location.file_name != nullptr ? location.file_name : "<unknown>";
    log_single(fn, "\0\0\0\0", file_name);
    log_single(fn, "\0\0\0\0", ":");
    log_single(fn, "\0\0\0\0", location.line);
    log_single(fn, "\0\0\0\0", ":");
    log_single(fn, "\0\0\0\0", location.column);
}

template <>
inline void log_single(LogFn fn, const char *, Value value) {
    if (value.type().kind() == TypeKind::Integer && !value.type().is_signed() && value.is_inline()) {
        log_single(fn, "\0\0\0\0", value.handle());
    } else {
        log_single(fn, "\0\0\0\0", "<unknown>");
    }
}

} // namespace ustd::detail
