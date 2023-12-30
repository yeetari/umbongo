#include <kernel/proc/process.hh>
#include <kernel/proc/thread.hh>
#include <kernel/sys_result.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel::arch {
namespace {

struct SyscallFrame {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t rflags;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rdx;
    uint64_t rip;
    uint64_t rbx;
    uint64_t rax;
};

using SyscallHandler = SyscallResult (Process::*)(uint64_t, uint64_t, uint64_t);
#define S(name, ...) reinterpret_cast<SyscallHandler>(&Process::sys_##name),
const ustd::Array s_syscall_table{
#include <kernel/api/syscalls.in>
};
#undef S

} // namespace

extern "C" void syscall_handler(SyscallFrame *frame, Thread *thread) {
    // Syscall number passed in rax.
    if (frame->rax >= s_syscall_table.size()) {
        return;
    }
    ASSERT_PEDANTIC(thread != nullptr);
    auto *process = &thread->process();
    ASSERT_PEDANTIC(process != nullptr);
    const auto result = (process->*s_syscall_table[frame->rax])(frame->rdi, frame->rsi, frame->rdx);
    frame->rax = result.value();
}

} // namespace kernel::arch
