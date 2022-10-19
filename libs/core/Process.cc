#include <core/Process.hh>

#include <system/Syscall.hh>
#include <ustd/Try.hh>

namespace core {

ustd::Result<void, ub_error_t> spawn_process(ustd::Span<const uint8_t> binary, ustd::StringView name) {
    TRY(system::syscall(SYS_proc_create, binary.data(), binary.size(), name.data(), name.length()));
    return {};
}

} // namespace core
