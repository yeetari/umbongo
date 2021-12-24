#include <boot/BootInfo.hh>
#include <boot/Efi.hh>
#include <elf/Elf.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Memory.hh>
#include <ustd/StringView.hh>
#include <ustd/Types.hh>

namespace {

constexpr usize k_kernel_stack_page_count = 32;

efi::SystemTable *s_st;

} // namespace

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    s_st->con_out->output_string(s_st->con_out, L"\r\nAssertion '");
    while (*expr != '\0') {
        ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*expr++), '\0'};
        s_st->con_out->output_string(s_st->con_out, array.data());
    }
    s_st->con_out->output_string(s_st->con_out, L"' failed at ");
    while (*file != '\0') {
        ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*file++), '\0'};
        s_st->con_out->output_string(s_st->con_out, array.data());
    }
    s_st->con_out->output_string(s_st->con_out, L":");
    ustd::Array<wchar_t, 20> line_buf{'\0'};
    usize line_len = 0;
    do {
        const char digit = static_cast<char>(line % 10);
        line_buf[line_len++] = static_cast<wchar_t>('0' + digit);
        line /= 10;
    } while (line > 0);
    for (usize i = line_len; i > 0; i--) {
        ustd::Array<wchar_t, 2> array{line_buf[i - 1], '\0'};
        s_st->con_out->output_string(s_st->con_out, array.data());
    }
    if (msg != nullptr) {
        s_st->con_out->output_string(s_st->con_out, L"\r\n=> ");
        while (*msg != '\0') {
            ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*msg++), '\0'};
            s_st->con_out->output_string(s_st->con_out, array.data());
        }
    }
    while (true) {
        asm volatile("cli; hlt");
    }
}

#define EFI_CHECK(expr, msg)                                                                                           \
    {                                                                                                                  \
        efi::Status status = (expr);                                                                                   \
        ENSURE(status == efi::Status::Success, (msg));                                                                 \
    }

static bool traverse_directory(RamFsEntry *&ram_fs, RamFsEntry *&current_entry, efi::FileProtocol *directory,
                               const char *path, usize path_length) {
    // Get size of file info struct.
    efi::FileInfo *info = nullptr;
    usize info_size = 0;
    directory->read(directory, &info_size, info);
    if (info_size == 0) {
        return false;
    }

    // Allocate memory for file info struct.
    EFI_CHECK(
        s_st->boot_services->allocate_pool(efi::MemoryType::LoaderData, info_size, reinterpret_cast<void **>(&info)),
        "Failed to allocate memory for file info struct!")
    EFI_CHECK(directory->read(directory, &info_size, info), "Failed to read file info!")

    const auto *name = static_cast<const wchar_t *>(info->name);
    usize name_length = 0;
    while (name[name_length] != '\0') {
        name_length++;
    }
    name_length++;
    ustd::StringView name_view(reinterpret_cast<const char *>(name));
    if (name_view == reinterpret_cast<const char *>(L"kernel") ||
        name_view == reinterpret_cast<const char *>(L"NvVars") || name_view == reinterpret_cast<const char *>(L".") ||
        name_view == reinterpret_cast<const char *>(L"..")) {
        EFI_CHECK(s_st->boot_services->free_pool(info), "Failed to free memory for file info struct!")
        return true;
    }

    // Open file for reading.
    efi::FileProtocol *file = nullptr;
    EFI_CHECK(directory->open(directory, &file, name, efi::FileMode::Read, efi::FileFlag::ReadOnly),
              "Failed to load file from disk!")
    ENSURE(file != nullptr, "Failed to load file from disk!");

    bool is_directory = (info->flags & efi::FileFlag::Directory) == efi::FileFlag::Directory;
    ENSURE(info->physical_size >= info->file_size);

    // Allocate memory for ramfs entry.
    const usize entry_size = sizeof(RamFsEntry) + path_length + 1 + name_length + (!is_directory ? info->file_size : 0);
    const usize page_count = (entry_size + 4096 - 1) / 4096;
    auto **next_entry_ptr = current_entry != nullptr ? &current_entry->next : &ram_fs;
    EFI_CHECK(s_st->boot_services->allocate_pages(efi::AllocateType::AllocateAnyPages, efi::MemoryType::LoaderData,
                                                  page_count, reinterpret_cast<uintptr *>(next_entry_ptr)),
              "Failed to allocate memory for ramfs entry!")
    current_entry = *next_entry_ptr;

    // Setup header.
    auto *header = new (*next_entry_ptr) RamFsEntry;
    header->name = reinterpret_cast<const char *>(&header->next) + sizeof(void *);
    header->data = !is_directory
                       ? reinterpret_cast<const uint8 *>(&header->next) + sizeof(void *) + path_length + 1 + name_length
                       : nullptr;
    header->data_size = info->file_size;
    header->is_directory = is_directory;

    // Copy name. We can't use memcpy since the UEFI file name is made up of wide chars.
    for (usize i = 0; i < path_length; i++) {
        const_cast<char *>(header->name)[i] = path[i];
    }
    const_cast<char *>(header->name)[path_length] = '/';
    for (usize i = 0; i < name_length; i++) {
        const_cast<char *>(header->name)[path_length + i + 1] = static_cast<char>(info->name[i]);
    }

    if (is_directory) {
        while (traverse_directory(ram_fs, current_entry, file, header->name, path_length + name_length)) {
        }
    } else {
        // Copy data.
        EFI_CHECK(file->read(file, &info->file_size, const_cast<uint8 *>(header->data)), "Failed to read from file!")
    }

    // Finally, close file and free memory for file info struct.
    EFI_CHECK(file->close(file), "Failed to close file!")
    EFI_CHECK(s_st->boot_services->free_pool(info), "Failed to free memory for file info struct!")
    return true;
}

efi::Status efi_main(efi::Handle image_handle, efi::SystemTable *st) {
    s_st = st;

    // Clear screen.
    st->con_out->clear_screen(st->con_out);

    // Retrieve loaded image protocol.
    efi::LoadedImageProtocol *loaded_image = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(image_handle, &efi::LoadedImageProtocol::guid,
                                                 reinterpret_cast<void **>(&loaded_image)),
              "Loaded image protocol not present!")
    ENSURE(loaded_image != nullptr, "Loaded image protocol not present!");

    // Retrieve file system protocol.
    efi::SimpleFileSystemProtocol *file_system = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(loaded_image->device_handle, &efi::SimpleFileSystemProtocol::guid,
                                                 reinterpret_cast<void **>(&file_system)),
              "File system protocol not present!")
    ENSURE(file_system != nullptr, "File system protocol not present!");

    // Open volume.
    efi::FileProtocol *root_directory = nullptr;
    s_st->con_out->output_string(s_st->con_out, L"Opening volume...\r\n");
    EFI_CHECK(file_system->open_volume(file_system, &root_directory), "Failed to open volume!")
    ENSURE(root_directory != nullptr, "Failed to open volume!");

    // Load kernel from disk.
    efi::FileProtocol *kernel_file = nullptr;
    s_st->con_out->output_string(s_st->con_out, L"Loading kernel from disk...\r\n");
    EFI_CHECK(
        root_directory->open(root_directory, &kernel_file, L"kernel", efi::FileMode::Read, efi::FileFlag::ReadOnly),
        "Failed to load kernel from disk!")
    ENSURE(kernel_file != nullptr, "Failed to load kernel from disk!");

    // Parse kernel ELF header.
    elf::Header kernel_header{};
    uint64 kernel_header_size = sizeof(elf::Header);
    s_st->con_out->output_string(s_st->con_out, L"Parsing kernel ELF header...\r\n");
    EFI_CHECK(kernel_file->read(kernel_file, &kernel_header_size, &kernel_header), "Failed to read kernel header!")
    ENSURE(kernel_header.magic[0] == 0x7f, "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[1] == 'E', "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[2] == 'L', "Kernel ELF header corrupt!");
    ENSURE(kernel_header.magic[3] == 'F', "Kernel ELF header corrupt!");

    // Allocate memory for and read kernel program headers.
    elf::ProgramHeader *phdrs = nullptr;
    uint64 phdrs_size = kernel_header.ph_count * kernel_header.ph_size;
    EFI_CHECK(
        st->boot_services->allocate_pool(efi::MemoryType::LoaderData, phdrs_size, reinterpret_cast<void **>(&phdrs)),
        "Failed to allocate memory for kernel program headers!")
    EFI_CHECK(kernel_file->set_position(kernel_file, static_cast<uint64>(kernel_header.ph_off)),
              "Failed to set position of kernel file!")
    EFI_CHECK(kernel_file->read(kernel_file, &phdrs_size, phdrs), "Failed to read kernel program headers!")

    // Parse kernel load program headers.
    s_st->con_out->output_string(s_st->con_out, L"Parsing kernel load program headers...\r\n");
    for (uint16 i = 0; i < kernel_header.ph_count; i++) {
        auto &phdr = phdrs[i];
        if (phdr.type != elf::SegmentType::Load) {
            continue;
        }
        const usize page_count = (phdr.memsz + 4096 - 1) / 4096;
        EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAddress, efi::MemoryType::Reserved,
                                                    page_count, &phdr.paddr),
                  "Failed to claim physical memory for kernel!")
        // Zero out any uninitialised data.
        if (phdr.filesz != phdr.memsz) {
            __builtin_memset(reinterpret_cast<void *>(phdr.paddr), 0, phdr.memsz);
        }
        kernel_file->set_position(kernel_file, static_cast<uint64>(phdr.offset));
        EFI_CHECK(kernel_file->read(kernel_file, &phdr.filesz, reinterpret_cast<void *>(phdr.paddr)),
                  "Failed to read kernel!")
    }
    EFI_CHECK(st->boot_services->free_pool(phdrs), "Failed to free memory for kernel program headers!")
    EFI_CHECK(kernel_file->close(kernel_file), "Failed to close kernel file!")

    RamFsEntry *ram_fs = nullptr;
    RamFsEntry *current_entry = nullptr;
    while (traverse_directory(ram_fs, current_entry, root_directory, nullptr, 0)) {
    }
    EFI_CHECK(root_directory->close(root_directory), "Failed to close directory!")

    // Allocate stack for kernel.
    uintptr kernel_stack = 0;
    EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAnyPages, efi::MemoryType::LoaderData,
                                                k_kernel_stack_page_count, &kernel_stack),
              "Failed to allocate memory for kernel stack!")
    ENSURE(kernel_stack != 0, "Failed to allocate memory for kernel stack!");

    // Find ACPI RSDP.
    void *rsdp = nullptr;
    s_st->con_out->output_string(s_st->con_out, L"Finding ACPI RSDP...\r\n");
    for (usize i = 0; i < st->configuration_table_count; i++) {
        auto &table = st->configuration_table[i];
        if (__builtin_memcmp(&table.vendor_guid, &efi::ConfigurationTable::acpi_guid, sizeof(efi::Guid)) == 0) {
            rsdp = table.vendor_table;
            break;
        }
    }
    ENSURE(rsdp != nullptr, "Failed to find ACPI RSDP!");

    // Get framebuffer info.
    efi::GraphicsOutputProtocol *gop = nullptr;
    s_st->con_out->output_string(s_st->con_out, L"Querying framebuffer info...\r\n");
    EFI_CHECK(st->boot_services->locate_protocol(&efi::GraphicsOutputProtocol::guid, nullptr,
                                                 reinterpret_cast<void **>(&gop)),
              "Failed to locate graphics output protocol!")
    ENSURE(gop != nullptr, "Failed to locate graphics output protocol!");

    // Find the best video mode.
    uint32 pixel_count = 0;
    uint32 preferred_mode = 0;
    for (uint32 i = 0; i < gop->mode->max_mode; i++) {
        efi::GraphicsOutputModeInfo *info = nullptr;
        usize info_size = 0;
        EFI_CHECK(gop->query_mode(gop, i, &info_size, &info), "Failed to query video mode!")
        if (info->width * info->height > pixel_count) {
            pixel_count = info->width * info->height;
            preferred_mode = i;
        }
    }

    // Set video mode.
    EFI_CHECK(gop->set_mode(gop, preferred_mode), "Failed to set video mode!")

    // Get EFI memory map data.
    usize map_key = 0;
    usize map_size = 0;
    usize descriptor_size = 0;
    uint32 descriptor_version = 0;
    s_st->con_out->output_string(s_st->con_out, L"Querying memory map size...\r\n");
    st->boot_services->get_memory_map(&map_size, nullptr, &map_key, &descriptor_size, &descriptor_version);
    ENSURE(descriptor_size >= sizeof(efi::MemoryDescriptor));

    // Allocate extra page for EFI memory map because we're about to allocate the memory for it + our memory map.
    // TODO: We should try to allocate in a loop whilst EfiStatus::BufferTooSmall is returned.
    map_size += 4096;

    // Allocate some memory for EFI memory map.
    efi::MemoryDescriptor *efi_map = nullptr;

    s_st->con_out->output_string(s_st->con_out, L"Allocating memory for EFI memory map...\r\n");
    EFI_CHECK(
        st->boot_services->allocate_pool(efi::MemoryType::LoaderData, map_size, reinterpret_cast<void **>(&efi_map)),
        "Failed to allocate memory for EFI memory map!")
    ENSURE(efi_map != nullptr, "Failed to allocate memory for EFI memory map!");

    // Allocate some memory for our memory map.
    const usize efi_map_entry_count = map_size / descriptor_size;
    MemoryMapEntry *map = nullptr;
    s_st->con_out->output_string(s_st->con_out, L"Allocating memory for memory map...\r\n");
    EFI_CHECK(st->boot_services->allocate_pool(efi::MemoryType::LoaderData,
                                               efi_map_entry_count * sizeof(MemoryMapEntry),
                                               reinterpret_cast<void **>(&map)),
              "Failed to allocate memory for memory map!")
    ENSURE(map != nullptr, "Failed to allocate memory for memory map!");

    // Retrieve EFI memory map.
    s_st->con_out->output_string(s_st->con_out, L"Retrieving EFI memory map...\r\n");
    EFI_CHECK(st->boot_services->get_memory_map(&map_size, efi_map, &map_key, &descriptor_size, &descriptor_version),
              "Failed to retrieve EFI memory map!")

    // We have to be extremely careful not to call any EFI functions between now and the exit_boot_services call. This
    // means that we can't free the EFI memory map memory, but the kernel can reclaim it later.

    // Construct memory map.
    for (usize i = 0, j = 0; i < map_size; i += descriptor_size, j++) {
        auto *descriptor = reinterpret_cast<efi::MemoryDescriptor *>(&reinterpret_cast<uint8 *>(efi_map)[i]);
        auto *entry = &map[j];
        switch (descriptor->type) {
        case efi::MemoryType::Conventional:
            entry->type = MemoryType::Available;
            break;
        case efi::MemoryType::BootServicesCode:
        case efi::MemoryType::BootServicesData:
        case efi::MemoryType::LoaderCode:
        case efi::MemoryType::LoaderData:
            // TODO: Mark efi::MemoryType::AcpiReclaimable as reclaimable too?
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
        .map_entry_count = map_size / descriptor_size - 1,
        .ram_fs = ram_fs,
    };

    // Exit boot services.
    EFI_CHECK(st->boot_services->exit_boot_services(image_handle, map_key), "Failed to exit boot services!")

    // Call kernel and spin on return.
    asm volatile("mov %0, %%rsp" : : "r"(kernel_stack + k_kernel_stack_page_count * 4_KiB) : "rsp");
    reinterpret_cast<__attribute__((sysv_abi)) void (*)(BootInfo *)>(kernel_header.entry)(&boot_info);
    while (true) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
