#include <kernel/proc/Process.hh>

#include <elf/Elf.hh>
#include <kernel/SysError.hh>
#include <kernel/SysResult.hh>
#include <kernel/SyscallTypes.hh>
#include <kernel/cpu/InterruptDisabler.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/fs/File.hh>
#include <kernel/fs/FileHandle.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/Optional.hh>
#include <ustd/SharedPtr.hh>
#include <ustd/String.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>
#include <ustd/Utility.hh>
#include <ustd/Vector.hh>

namespace {

usize s_pid_counter = 0;

String interpreter_for(File &file) {
    // TODO: Check file read success.
    elf::Header header{};
    file.read({&header, sizeof(elf::Header)});
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        file.read({&phdr, sizeof(elf::ProgramHeader)}, static_cast<usize>(header.ph_off + header.ph_size * i));
        if (phdr.type == elf::SegmentType::Interp) {
            // TODO: Proper validation.
            String path(phdr.filesz - 1);
            file.read({path.data(), phdr.filesz - 1}, phdr.offset);
            return path;
        }
    }
    return {};
}

} // namespace

Process *Process::create_kernel() {
    return new Process(s_pid_counter++, true, SharedPtr<VirtSpace>(MemoryManager::kernel_space()));
}

Process *Process::create_user() {
    return new Process(s_pid_counter++, false, MemoryManager::kernel_space()->clone());
}

Process::Process(usize pid, bool is_kernel, SharedPtr<VirtSpace> virt_space)
    : m_pid(pid), m_is_kernel(is_kernel), m_virt_space(ustd::move(virt_space)) {
    m_register_state.cs = is_kernel ? 0x08 : (0x20u | 0x3u);
    m_register_state.ss = is_kernel ? 0x10 : (0x18u | 0x3u);
    m_register_state.rflags = 0x202;
    if (is_kernel) {
        m_register_state.rsp = reinterpret_cast<uintptr>(new char[4_KiB] + 4_KiB);
    }
}

Process::~Process() {
    m_prev->m_next = m_next;
    m_next->m_prev = m_prev;
}

uint32 Process::allocate_fd() {
    // TODO: Limits/error handling. Allocating 2^32 fds would overflow the vector.
    for (uint32 i = 0; i < m_fds.size(); i++) {
        if (!m_fds[i]) {
            return i;
        }
    }
    m_fds.emplace();
    return m_fds.size() - 1;
}

SysResult Process::exec(StringView path, const Vector<String> &args) {
    auto file = Vfs::open(path, OpenMode::None);
    if (!file) {
        return SysError::NonExistent;
    }
    // TODO: Proper validation.
    auto &stack_region = m_virt_space->allocate_region(2_MiB, RegionAccess::Writable | RegionAccess::UserAccessible);
    m_register_state.rsp = stack_region.base() + stack_region.size();

    // We don't want to be interrupted whilst we have our virt space mapped in to copy the executable data.
    auto *original_space = MemoryManager::current_space();
    InterruptDisabler disabler;
    MemoryManager::switch_space(*m_virt_space);

    auto interpreter_path = interpreter_for(*file);
    auto executable = !interpreter_path.empty() ? Vfs::open(interpreter_path.view(), OpenMode::None) : file;

    // TODO: Check file read success.
    elf::Header header{};
    executable->read({&header, sizeof(elf::Header)});
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        executable->read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i);
        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }
        ASSERT(phdr.filesz <= phdr.memsz);

        RegionAccess access = RegionAccess::UserAccessible;
        if ((phdr.flags & elf::SegmentFlags::Executable) == elf::SegmentFlags::Executable) {
            access |= RegionAccess::Executable;
        }
        // TODO: Don't always map writable. We only need to for copying the data in.
        access |= RegionAccess::Writable;
        auto &region = m_virt_space->allocate_region(phdr.memsz + (phdr.vaddr & 0xfffu), access);

        // Segment contains entry point.
        if (header.entry >= phdr.vaddr && header.entry < phdr.vaddr + phdr.memsz) {
            m_register_state.rip = region.base() + header.entry - (phdr.vaddr & ~4095u);
        }

        usize copy_offset = phdr.vaddr & 0xfffu;
        ASSERT(region.base() + copy_offset + phdr.memsz <= region.base() + region.size());
        executable->read({reinterpret_cast<void *>(region.base() + copy_offset), phdr.filesz}, phdr.offset);
    }

    // Setup user stack.
    Vector<uintptr> argv;
    auto push_arg = [&](StringView arg) {
        m_register_state.rsp -= round_up(arg.length() + 1, sizeof(usize));
        memcpy(reinterpret_cast<void *>(m_register_state.rsp), arg.data(), arg.length());
        *(reinterpret_cast<char *>(m_register_state.rsp) + arg.length()) = '\0';
        argv.push(m_register_state.rsp);
    };
    if (!interpreter_path.empty()) {
        push_arg(path);
    }
    for (const auto &arg : args) {
        push_arg(arg.view());
    }

    for (uint32 i = 0; i < argv.size(); i++) {
        m_register_state.rsp -= sizeof(uintptr);
        *reinterpret_cast<uintptr *>(m_register_state.rsp) = argv[argv.size() - i - 1];
    }

    m_register_state.rdi = argv.size();          // argc
    m_register_state.rsi = m_register_state.rsp; // argv

    // Allocate some space for heap storage.
    m_virt_space->create_region(6_TiB, 2_MiB, RegionAccess::Writable | RegionAccess::UserAccessible);
    MemoryManager::switch_space(*original_space);
    return 0;
}

void Process::kill() {
    m_state = ProcessState::Dead;
}

void Process::set_entry_point(uintptr entry) {
    m_register_state.rip = entry;
}
