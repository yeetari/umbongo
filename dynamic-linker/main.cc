#include <core/file.hh>
#include <elf/elf.hh>
#include <log/log.hh>
#include <system/syscall.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>

size_t main(size_t argc, const char **argv) {
    auto file = EXPECT(core::File::open(argv[0]));
    auto header = EXPECT(file.read<elf::Header>());

    uintptr_t region_base = ustd::Limits<uintptr_t>::max();
    uintptr_t region_end = 0;
    for (uint16_t i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        EXPECT(file.read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i));
        if (phdr.type == elf::SegmentType::Load) {
            ASSERT(phdr.filesz <= phdr.memsz);
            region_base = ustd::min(region_base, phdr.vaddr);
            region_end = ustd::max(region_end, phdr.vaddr + phdr.memsz);
        }
    }

    // TODO: Don't always allocate writable + executable.
    auto base_offset = EXPECT(system::syscall<uintptr_t>(UB_SYS_allocate_region, region_end - region_base,
                                                         UB_MEMORY_PROT_WRITE | UB_MEMORY_PROT_EXEC));
    size_t dynamic_entry_count = 0;
    size_t dynamic_table_offset = 0;
    for (uint16_t i = 0; i < header.ph_count; i++) {
        elf::ProgramHeader phdr{};
        EXPECT(file.read({&phdr, sizeof(elf::ProgramHeader)}, header.ph_off + header.ph_size * i));
        if (phdr.type == elf::SegmentType::Dynamic) {
            ASSERT(phdr.filesz % sizeof(elf::DynamicEntry) == 0);
            dynamic_entry_count = phdr.filesz / sizeof(elf::DynamicEntry);
            dynamic_table_offset = phdr.offset;
        } else if (phdr.type == elf::SegmentType::Load) {
            // TODO: Only bother zeroing bss.
            __builtin_memset(reinterpret_cast<void *>(base_offset + phdr.vaddr), 0, phdr.memsz);
            EXPECT(file.read({reinterpret_cast<void *>(base_offset + phdr.vaddr), phdr.filesz}, phdr.offset));
        }
    }

    size_t relocation_entry_size = 0;
    size_t relocation_table_offset = 0;
    size_t relocation_table_size = 0;
    for (size_t i = 0; i < dynamic_entry_count; i++) {
        elf::DynamicEntry entry{};
        EXPECT(file.read({&entry, sizeof(elf::DynamicEntry)}, dynamic_table_offset + sizeof(elf::DynamicEntry) * i));
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
            log::error("Unknown dynamic entry type {} in program {}", static_cast<int64_t>(entry.type), argv[0]);
            ENSURE_NOT_REACHED();
        }
    }

    if (relocation_entry_size != 0 || relocation_table_offset != 0 || relocation_table_size != 0) {
        ASSERT(relocation_entry_size != 0);
        ASSERT(relocation_table_size != 0);
        ASSERT(relocation_table_size % relocation_entry_size == 0);
    }
    size_t relocation_entry_count = relocation_entry_size != 0 ? relocation_table_size / relocation_entry_size : 0;
    for (size_t i = 0; i < relocation_entry_count; i++) {
        elf::Rela rela{};
        EXPECT(file.read({&rela, sizeof(elf::Rela)}, relocation_table_offset + relocation_entry_size * i));

        auto *ptr = reinterpret_cast<size_t *>(base_offset + rela.offset);
        auto type = static_cast<elf::RelocationType>(rela.info & 0xffffffffu);
        switch (type) {
        case elf::RelocationType::None:
            break;
        case elf::RelocationType::Relative:
            *ptr = static_cast<size_t>(static_cast<ssize_t>(base_offset) + rela.addend);
            break;
        default:
            log::error("Unknown relocation type {} in program {}", static_cast<uint32_t>(type), argv[0]);
            ENSURE_NOT_REACHED();
        }
    }

    const uintptr_t entry_point = base_offset + header.entry;
    reinterpret_cast<void (*)(size_t, const char **)>(entry_point)(argc - 1, &argv[1]);
    ENSURE_NOT_REACHED();
}
