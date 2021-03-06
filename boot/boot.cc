#include <boot/BootInfo.hh>
#include <boot/Efi.hh>
#include <libelf/Elf.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_kernel_stack_page_count = 8;

const EfiGuid g_acpi_table_guid{0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}};

const EfiGuid g_graphics_output_protocol_guid{
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xdE, 0xd0, 0x80, 0x51, 0x6a}};

const EfiGuid g_loaded_image_protocol_guid{
    0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

const EfiGuid g_simple_file_system_protocol_guid{
    0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

EfiSystemTable *s_st;

} // namespace

void put_char(char ch) {
    if (ch == '\n') {
        s_st->con_out->output_string(s_st->con_out, L"\r\n");
        return;
    }
    Array<wchar_t, 2> array{static_cast<wchar_t>(ch), '\0'};
    s_st->con_out->output_string(s_st->con_out, array.data());
}

#define EFI_CHECK(expr, msg)                                                                                           \
    {                                                                                                                  \
        EfiStatus status = (expr);                                                                                     \
        ENSURE(status == EfiStatus::Success, (msg));                                                                   \
    }

EfiStatus efi_main(EfiHandle image_handle, EfiSystemTable *st) {
    s_st = st;

    // Flush input buffer and clear screen.
    st->con_in->reset(st->con_in, false);
    st->con_out->clear_screen(st->con_out);

    // Retrieve loaded image protocol.
    EfiLoadedImageProtocol *loaded_image = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(image_handle, &g_loaded_image_protocol_guid,
                                                 reinterpret_cast<void **>(&loaded_image)),
              "Loaded image protocol not present!")
    ENSURE(loaded_image != nullptr, "Loaded image protocol not present!");

    // Retrieve file system protocol.
    EfiSimpleFileSystemProtocol *file_system = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(loaded_image->device_handle, &g_simple_file_system_protocol_guid,
                                                 reinterpret_cast<void **>(&file_system)),
              "File system protocol not present!")
    ENSURE(file_system != nullptr, "File system protocol not present!");

    // Open volume.
    EfiFileProtocol *directory = nullptr;
    logln("Opening volume...");
    EFI_CHECK(file_system->open_volume(file_system, &directory), "Failed to open volume!")
    ENSURE(directory != nullptr, "Failed to open volume!");

    // Load kernel from disk.
    EfiFileProtocol *kernel_file = nullptr;
    logln("Loading kernel from disk...");
    EFI_CHECK(directory->open(directory, &kernel_file, L"kernel", EfiFileMode::Read, EfiFileFlag::ReadOnly),
              "Failed to load kernel from disk!")
    ENSURE(kernel_file != nullptr, "Failed to load kernel from disk!");

    // Parse kernel ELF header.
    elf::Header kernel_header{};
    uint64 kernel_header_size = sizeof(elf::Header);
    logln("Parsing kernel ELF header...");
    EFI_CHECK(kernel_file->read(kernel_file, &kernel_header_size, &kernel_header), "Failed to read kernel header!")
    ENSURE(kernel_header.magic[0] == 0x7f, "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[1] == 'E', "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[2] == 'L', "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[3] == 'F', "Kernel ELF header corrupt!");

    // Allocate memory for and read kernel program headers.
    elf::ProgramHeader *phdrs = nullptr;
    uint64 phdrs_size = kernel_header.ph_count * kernel_header.ph_size;
    EFI_CHECK(
        st->boot_services->allocate_pool(EfiMemoryType::LoaderData, phdrs_size, reinterpret_cast<void **>(&phdrs)),
        "Failed to allocate memory for kernel program headers!")
    EFI_CHECK(kernel_file->set_position(kernel_file, static_cast<uint64>(kernel_header.ph_off)),
              "Failed to set position of kernel file!")
    EFI_CHECK(kernel_file->read(kernel_file, &phdrs_size, phdrs), "Failed to read kernel program headers!")

    // Parse kernel load program headers.
    logln("Parsing kernel load program headers...");
    for (uint16 i = 0; i < kernel_header.ph_count; i++) {
        auto &phdr = phdrs[i];
        if (phdr.type != elf::ProgramHeaderType::Load) {
            continue;
        }
        const usize page_count = (phdr.memsz + 4096 - 1) / 4096;
        EFI_CHECK(st->boot_services->allocate_pages(EfiAllocateType::AllocateAddress, EfiMemoryType::LoaderData,
                                                    page_count, &phdr.paddr),
                  "Failed to claim physical memory for kernel!")
        // Zero out any uninitialised data.
        if (phdr.filesz != phdr.memsz) {
            memset(reinterpret_cast<void *>(phdr.paddr), 0, phdr.memsz);
        }
        kernel_file->set_position(kernel_file, static_cast<uint64>(phdr.offset));
        EFI_CHECK(kernel_file->read(kernel_file, &phdr.filesz, reinterpret_cast<void *>(phdr.paddr)),
                  "Failed to read kernel!")
    }
    EFI_CHECK(st->boot_services->free_pool(phdrs), "Failed to free memory for kernel program headers!")
    EFI_CHECK(kernel_file->close(kernel_file), "Failed to close kernel file!")

    // Load files from disk into ramfs.
    RamFsEntry *ram_fs = nullptr;
    RamFsEntry *current_entry = nullptr;
    while (true) {
        // Get size of file info struct.
        EfiFileInfo *info = nullptr;
        usize info_size = 0;
        directory->read(directory, &info_size, info);
        if (info_size == 0) {
            break;
        }

        // Allocate memory for file info struct.
        EFI_CHECK(
            st->boot_services->allocate_pool(EfiMemoryType::LoaderData, info_size, reinterpret_cast<void **>(&info)),
            "Failed to allocate memory for file info struct!")
        EFI_CHECK(directory->read(directory, &info_size, info), "Failed to read file info!")

        if ((info->flags & EfiFileFlag::Directory) == EfiFileFlag::Directory) {
            EFI_CHECK(st->boot_services->free_pool(info), "Failed to free memory for file info struct!")
            continue;
        }

        const auto *name = static_cast<const wchar_t *>(info->name);
        const usize name_length = wstrlen(name) + 1;
        if (strcmp(reinterpret_cast<const char *>(name), reinterpret_cast<const char *>(L"kernel")) == 0 ||
            strcmp(reinterpret_cast<const char *>(name), reinterpret_cast<const char *>(L"NvVars")) == 0) {
            EFI_CHECK(st->boot_services->free_pool(info), "Failed to free memory for file info struct!")
            continue;
        }

        // Allocate memory for ramfs entry.
        const usize entry_size = sizeof(RamFsEntry) + name_length + info->physical_size;
        const usize page_count = (entry_size + 4096 - 1) / 4096;
        auto **next_entry_ptr = current_entry != nullptr ? &current_entry->next : &ram_fs;
        EFI_CHECK(st->boot_services->allocate_pages(EfiAllocateType::AllocateAnyPages, EfiMemoryType::LoaderData,
                                                    page_count, reinterpret_cast<uintptr *>(next_entry_ptr)),
                  "Failed to allocate memory for ramfs entry!")
        current_entry = *next_entry_ptr;

        // Setup header.
        auto *header = new (*next_entry_ptr) RamFsEntry;
        header->name = reinterpret_cast<const char *>(&header->next) + sizeof(void *);
        header->data = reinterpret_cast<const uint8 *>(&header->next) + sizeof(void *) + name_length;

        // Copy name. We can't use memcpy since the UEFI file name is made up of wide chars.
        for (usize i = 0; i < name_length; i++) {
            const_cast<char *>(header->name)[i] = static_cast<char>(info->name[i]);
        }

        // Open file for reading.
        EfiFileProtocol *file = nullptr;
        EFI_CHECK(directory->open(directory, &file, name, EfiFileMode::Read, EfiFileFlag::ReadOnly),
                  "Failed to load file from disk!")
        ENSURE(file != nullptr, "Failed to load file from disk!");

        // Copy data.
        EFI_CHECK(file->read(file, &info->file_size, const_cast<uint8 *>(header->data)), "Failed to read from file!")

        // Finally close file and free memory for file info struct.
        EFI_CHECK(file->close(file), "Failed to close file!")
        EFI_CHECK(st->boot_services->free_pool(info), "Failed to free memory for file info struct!")
    }
    EFI_CHECK(directory->close(directory), "Failed to close directory!")

    // Allocate stack for kernel.
    uintptr kernel_stack = 0;
    EFI_CHECK(st->boot_services->allocate_pages(EfiAllocateType::AllocateAnyPages, EfiMemoryType::LoaderData,
                                                k_kernel_stack_page_count, &kernel_stack),
              "Failed to allocate memory for kernel stack!")
    ENSURE(kernel_stack != 0, "Failed to allocate memory for kernel stack!");

    // Find ACPI RSDP.
    void *rsdp = nullptr;
    logln("Finding ACPI RSDP...");
    for (usize i = 0; i < st->configuration_table_count; i++) {
        auto &table = st->configuration_table[i];
        if (memcmp(&table.vendor_guid, &g_acpi_table_guid, sizeof(EfiGuid)) == 0) {
            rsdp = table.vendor_table;
            break;
        }
    }
    ENSURE(rsdp != nullptr, "Failed to find ACPI RSDP!");

    // Get framebuffer info.
    EfiGraphicsOutputProtocol *gop = nullptr;
    logln("Querying framebuffer info...");
    EFI_CHECK(
        st->boot_services->locate_protocol(&g_graphics_output_protocol_guid, nullptr, reinterpret_cast<void **>(&gop)),
        "Failed to locate graphics output protocol!")
    ENSURE(gop != nullptr, "Failed to locate graphics output protocol!");

    // Find best video mode.
    uint32 pixel_count = 0;
    uint32 preferred_mode = 0;
    for (uint32 i = 0; i < gop->mode->max_mode; i++) {
        EfiGraphicsOutputModeInfo *info = nullptr;
        usize info_size = 0;
        EFI_CHECK(gop->query_mode(gop, i, &info_size, &info), "Failed to query video mode!")
        if (info->width * info->height > pixel_count) {
            pixel_count = info->width * info->height;
            preferred_mode = i;
        }
    }

    // Set video mode.
    logln("Selecting mode {}", preferred_mode);
    EFI_CHECK(gop->set_mode(gop, preferred_mode), "Failed to set video mode!")

    // Get EFI memory map data.
    usize map_key = 0;
    usize map_size = 0;
    usize descriptor_size = 0;
    uint32 descriptor_version = 0;
    logln("Querying memory map size...");
    st->boot_services->get_memory_map(&map_size, nullptr, &map_key, &descriptor_size, &descriptor_version);
    ENSURE(descriptor_size >= sizeof(EfiMemoryDescriptor));

    // Allocate extra page for EFI memory map because we're about to allocate the memory for it + our memory map.
    // TODO: We should try to allocate in a loop whilst EfiStatus::BufferTooSmall is returned.
    map_size += 4096;

    // Allocate some memory for EFI memory map.
    EfiMemoryDescriptor *efi_map = nullptr;
    logln("Allocating memory for EFI memory map...");
    EFI_CHECK(
        st->boot_services->allocate_pool(EfiMemoryType::LoaderData, map_size, reinterpret_cast<void **>(&efi_map)),
        "Failed to allocate memory for EFI memory map!")
    ENSURE(efi_map != nullptr, "Failed to allocate memory for EFI memory map!");

    // Allocate some memory for our memory map.
    const usize efi_map_entry_count = map_size / descriptor_size;
    MemoryMapEntry *map = nullptr;
    logln("Allocating memory for memory map...");
    EFI_CHECK(st->boot_services->allocate_pool(EfiMemoryType::LoaderData, efi_map_entry_count * sizeof(MemoryMapEntry),
                                               reinterpret_cast<void **>(&map)),
              "Failed to allocate memory for memory map!")
    ENSURE(map != nullptr, "Failed to allocate memory for memory map!");

    // Retrieve EFI memory map.
    logln("Retrieving EFI memory map...");
    EFI_CHECK(st->boot_services->get_memory_map(&map_size, efi_map, &map_key, &descriptor_size, &descriptor_version),
              "Failed to retrieve EFI memory map!")

    // We have to be extremely careful not to call any EFI functions between now and the exit_boot_services call. This
    // means that we can't free the EFI memory map memory, but the kernel can reclaim it later.

    // Construct memory map.
    for (usize i = 0, j = 0; i < map_size; i += descriptor_size, j++) {
        auto *descriptor = reinterpret_cast<EfiMemoryDescriptor *>(&reinterpret_cast<uint8 *>(efi_map)[i]);
        auto *entry = &map[j];
        switch (descriptor->type) {
        case EfiMemoryType::Conventional:
            entry->type = MemoryType::Available;
            break;
        case EfiMemoryType::AcpiReclaimable:
            // TODO: Mark LoaderCode+LoaderData as reclaimable too.
            entry->type = MemoryType::Reclaimable;
            break;
        default:
            // Assume that anything unknown is reserved.
            entry->type = MemoryType::Reserved;
            break;
        }
        entry->base = descriptor->phys_start;
        entry->page_count = descriptor->page_count;
    }

    // Fill out boot info.
    BootInfo boot_info{
        .rsdp = rsdp,
        .width = gop->mode->info->width,
        .height = gop->mode->info->height,
        .pixels_per_scan_line = gop->mode->info->pixels_per_scan_line,
        .framebuffer_base = gop->mode->framebuffer_base,
        .map = map,
        // Recalculate map entry count here. We can't use efi_map_entry_count since the memory for the memory map needs
        // to be overallocated to store an entry for itself.
        .map_entry_count = map_size / descriptor_size,
        .ram_fs = ram_fs,
    };

    // Exit boot services.
    EFI_CHECK(st->boot_services->exit_boot_services(image_handle, map_key), "Failed to exit boot services!")

    // Call kernel and spin on return.
    asm volatile("mov %0, %%rsp" : : "r"(kernel_stack) : "rsp");
    reinterpret_cast<__attribute__((sysv_abi)) void (*)(BootInfo *)>(kernel_header.entry)(&boot_info);
    while (true) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
