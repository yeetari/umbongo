#include <core/File.hh>
#include <elf/Elf.hh>
#include <kernel/Syscall.hh>
#include <kernel/SyscallTypes.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Numeric.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

usize main(usize argc, const char **argv) {
    core::File file(argv[0]);
    if (!file) {
        return 1;
    }

    elf::Header header{};
    file.read({&header, sizeof(elf::Header)});

    uintptr region_base = Limits<uintptr>::max();
    uintptr region_end = 0;
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        file.read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i);
        if (phdr.type == elf::SegmentType::Load) {
            ASSERT(phdr.filesz <= phdr.memsz);
            region_base = ustd::min(region_base, phdr.vaddr);
            region_end = ustd::max(region_end, phdr.vaddr + phdr.memsz);
        }
    }

    // TODO: Don't always allocate writable + executable.
    auto base_offset = Syscall::invoke<uintptr>(Syscall::allocate_region, region_end - region_base,
                                                MemoryProt::Write | MemoryProt::Exec);
    usize dynamic_entry_count = 0;
    usize dynamic_table_offset = 0;
    for (uint16 i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        file.read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i);
        if (phdr.type == elf::SegmentType::Dynamic) {
            ASSERT(phdr.filesz % sizeof(elf::DynamicEntry) == 0);
            dynamic_entry_count = phdr.filesz / sizeof(elf::DynamicEntry);
            dynamic_table_offset = phdr.offset;
        } else if (phdr.type == elf::SegmentType::Load) {
            // TODO: Only bother zeroing bss.
            memset(reinterpret_cast<void *>(base_offset + phdr.vaddr), 0, phdr.memsz);
            file.read({reinterpret_cast<void *>(base_offset + phdr.vaddr), phdr.filesz}, phdr.offset);
        }
    }

    usize relocation_entry_size = 0;
    usize relocation_table_offset = 0;
    usize relocation_table_size = 0;
    for (usize i = 0; i < dynamic_entry_count; i++) {
        elf::DynamicEntry entry{};
        file.read({&entry, sizeof(elf::DynamicEntry)}, dynamic_table_offset + sizeof(elf::DynamicEntry) * i);
        switch (entry.type) {
        case elf::DynamicType::Null:
        case elf::DynamicType::Hash:
        case elf::DynamicType::StrTab:
        case elf::DynamicType::SymTab:
        case elf::DynamicType::StrSz:
        case elf::DynamicType::SymEnt:
        case elf::DynamicType::Debug:
        case elf::DynamicType::GnuHash:
        case elf::DynamicType::RelaCount:
        case elf::DynamicType::Flags1:
            break;
        case elf::DynamicType::Rela:
            relocation_table_offset = entry.value;
            break;
        case elf::DynamicType::RelaSz:
            relocation_table_size = entry.value;
            break;
        case elf::DynamicType::RelaEnt:
            relocation_entry_size = entry.value;
            break;
        default:
            dbgln("Unknown dynamic entry type {} in program {}", static_cast<int64>(entry.type), argv[0]);
            ENSURE_NOT_REACHED();
        }
    }

    if (relocation_entry_size != 0 || relocation_table_offset != 0 || relocation_table_size != 0) {
        ASSERT(relocation_entry_size != 0);
        ASSERT(relocation_table_size != 0);
        ASSERT(relocation_table_size % relocation_entry_size == 0);
    }
    usize relocation_entry_count = relocation_entry_size != 0 ? relocation_table_size / relocation_entry_size : 0;
    for (usize i = 0; i < relocation_entry_count; i++) {
        elf::Rela rela{};
        file.read({&rela, sizeof(elf::Rela)}, relocation_table_offset + relocation_entry_size * i);

        auto *ptr = reinterpret_cast<usize *>(base_offset + rela.offset);
        auto type = static_cast<elf::RelocationType>(rela.info & 0xffffffffu);
        switch (type) {
        case elf::RelocationType::None:
            break;
        case elf::RelocationType::Relative:
            *ptr = static_cast<usize>(static_cast<ssize>(base_offset) + rela.addend);
            break;
        default:
            dbgln("Unknown relocation type {} in program {}", static_cast<uint32>(type), argv[0]);
            ENSURE_NOT_REACHED();
        }
    }

    const uintptr entry_point = base_offset + header.entry;
    reinterpret_cast<void (*)(usize, const char **)>(entry_point)(argc - 1, &argv[1]);
    ENSURE_NOT_REACHED();
}
