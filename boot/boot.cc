#include <boot/boot_info.hh>
#include <boot/efi.hh>
#include <elf/elf.hh>
#include <ustd/algorithm.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/numeric.hh>
#include <ustd/ring_buffer.hh>
#include <ustd/span.hh>
#include <ustd/string_view.hh>
#include <ustd/types.hh>
#include <ustd/utility.hh>

namespace {

constexpr size_t k_kernel_stack_page_count = 32;

efi::SimpleTextOutputProtocol *s_con_out;

} // namespace

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    s_con_out->output_string(s_con_out, L"\r\nAssertion '");
    while (*expr != '\0') {
        ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*expr++), '\0'};
        s_con_out->output_string(s_con_out, array.data());
    }
    s_con_out->output_string(s_con_out, L"' failed at ");
    while (*file != '\0') {
        ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*file++), '\0'};
        s_con_out->output_string(s_con_out, array.data());
    }
    s_con_out->output_string(s_con_out, L":");
    ustd::Array<wchar_t, 20> line_buf{'\0'};
    size_t line_len = 0;
    do {
        const char digit = static_cast<char>(line % 10);
        line_buf[line_len++] = static_cast<wchar_t>('0' + digit);
        line /= 10;
    } while (line > 0);
    for (size_t i = line_len; i > 0; i--) {
        ustd::Array<wchar_t, 2> array{line_buf[i - 1], '\0'};
        s_con_out->output_string(s_con_out, array.data());
    }
    if (msg != nullptr) {
        s_con_out->output_string(s_con_out, L"\r\n=> ");
        while (*msg != '\0') {
            ustd::Array<wchar_t, 2> array{static_cast<wchar_t>(*msg++), '\0'};
            s_con_out->output_string(s_con_out, array.data());
        }
    }
    while (true) {
        asm volatile("cli; hlt");
    }
}

#define EFI_CHECK(expr, ...)                                                                                           \
    do {                                                                                                               \
        efi::Status status = (expr);                                                                                   \
        ENSURE(status == efi::Status::Success __VA_OPT__(, ) __VA_ARGS__);                                             \
    } while (false)

#define EFI_CHECK_PTR(expr, ptr, ...)                                                                                  \
    EFI_CHECK(expr __VA_OPT__(, ) __VA_ARGS__);                                                                        \
    ENSURE((ptr) != nullptr __VA_OPT__(, ) __VA_ARGS__)

static bool traverse_directory(efi::SystemTable *st, RamFsEntry *&ram_fs, RamFsEntry *&current_entry,
                               efi::FileProtocol *directory, ustd::StringView path) {
    // Get size of file info struct.
    efi::FileInfo *info = nullptr;
    size_t info_size = 0;
    directory->read(directory, &info_size, info);
    if (info_size == 0) {
        return false;
    }

    // Allocate memory for file info struct.
    EFI_CHECK(
        st->boot_services->allocate_pool(efi::MemoryType::LoaderData, info_size, reinterpret_cast<void **>(&info)),
        "Failed to allocate memory for file info struct!");
    EFI_CHECK(directory->read(directory, &info_size, info), "Failed to read file info!");

    const auto *name = static_cast<const wchar_t *>(info->name);
    size_t name_length = 0;
    while (name[name_length] != '\0') {
        name_length++;
    }
    name_length++;
    ustd::StringView name_view(reinterpret_cast<const char *>(name));
    if (name_view == reinterpret_cast<const char *>(L"kernel") ||
        name_view == reinterpret_cast<const char *>(L"NvVars") || name_view == reinterpret_cast<const char *>(L".") ||
        name_view == reinterpret_cast<const char *>(L"..")) {
        EFI_CHECK(st->boot_services->free_pool(info), "Failed to free memory for file info struct!");
        return true;
    }

    // Open file for reading.
    efi::FileProtocol *file = nullptr;
    EFI_CHECK_PTR(directory->open(directory, &file, name, efi::FileMode::Read, efi::FileFlag::ReadOnly), file,
                  "Failed to load file from disk!");

    bool is_directory = (info->flags & efi::FileFlag::Directory) == efi::FileFlag::Directory;
    ENSURE(info->physical_size >= info->file_size);

    // Allocate memory for ramfs entry.
    const size_t entry_size =
        sizeof(RamFsEntry) + path.length() + 1 + name_length + (!is_directory ? info->file_size : 0);
    const auto page_count = ustd::ceil_div(entry_size, 4_KiB);
    auto **next_entry_ptr = current_entry != nullptr ? &current_entry->next : &ram_fs;
    EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAnyPages, efi::MemoryType::LoaderData,
                                                page_count, reinterpret_cast<uintptr_t *>(next_entry_ptr)),
              "Failed to allocate memory for ramfs entry!");
    current_entry = *next_entry_ptr;

    // Setup header.
    auto *header = new (*next_entry_ptr) RamFsEntry;
    header->name = reinterpret_cast<const char *>(&header->next) + sizeof(void *);
    header->data = !is_directory ? reinterpret_cast<const uint8_t *>(&header->next) + sizeof(void *) + path.length() +
                                       1 + name_length
                                 : nullptr;
    header->data_size = info->file_size;
    header->is_directory = is_directory;

    // Copy name. We can't use memcpy since the UEFI file name is made up of wide chars.
    for (size_t i = 0; i < path.length(); i++) {
        const_cast<char *>(header->name)[i] = path[i];
    }
    const_cast<char *>(header->name)[path.length()] = '/';
    for (size_t i = 0; i < name_length; i++) {
        const_cast<char *>(header->name)[path.length() + i + 1] = static_cast<char>(info->name[i]);
    }

    if (!is_directory) {
        // Copy file data.
        EFI_CHECK(file->read(file, &info->file_size, const_cast<uint8_t *>(header->data)), "Failed to read file!");
    } else {
        while (traverse_directory(st, ram_fs, current_entry, file, {header->name, path.length() + name_length})) {
        }
    }

    // Finally, close file and free memory for file info struct.
    EFI_CHECK(file->close(file), "Failed to close file!");
    EFI_CHECK(st->boot_services->free_pool(info), "Failed to free memory for file info struct!");
    return true;
}

efi::Status efi_main(efi::Handle image_handle, efi::SystemTable *st) {
    // Set global console out pointer for use in the assertion handler.
    s_con_out = st->con_out;

    // Clear screen.
    st->con_out->clear_screen(st->con_out);

    // Retrieve loaded image protocol.
    efi::LoadedImageProtocol *loaded_image = nullptr;
    EFI_CHECK_PTR(st->boot_services->handle_protocol(image_handle, &efi::LoadedImageProtocol::guid,
                                                     reinterpret_cast<void **>(&loaded_image)),
                  loaded_image, "Loaded image protocol not present!");

    // Retrieve file system protocol.
    efi::SimpleFileSystemProtocol *file_system = nullptr;
    EFI_CHECK_PTR(st->boot_services->handle_protocol(loaded_image->device_handle, &efi::SimpleFileSystemProtocol::guid,
                                                     reinterpret_cast<void **>(&file_system)),
                  file_system, "File system protocol not present!");

    // Open volume.
    efi::FileProtocol *root_directory = nullptr;
    st->con_out->output_string(st->con_out, L"Opening volume...\r\n");
    EFI_CHECK_PTR(file_system->open_volume(file_system, &root_directory), root_directory, "Failed to open volume!");

    // Load kernel binary.
    efi::FileProtocol *kernel_file = nullptr;
    st->con_out->output_string(st->con_out, L"Loading kernel binary...\r\n");
    EFI_CHECK_PTR(
        root_directory->open(root_directory, &kernel_file, L"kernel", efi::FileMode::Read, efi::FileFlag::ReadOnly),
        kernel_file, "Failed to load kernel binary!");

    // Parse kernel ELF header.
    elf::Header kernel_header{};
    uint64_t kernel_header_size = sizeof(elf::Header);
    st->con_out->output_string(st->con_out, L"Parsing kernel ELF header...\r\n");
    EFI_CHECK(kernel_file->read(kernel_file, &kernel_header_size, &kernel_header), "Failed to read kernel header!");
    ENSURE(ustd::equal(kernel_header.magic, elf::k_magic), "Kernel ELF header corrupt!");

    // Read single program header.
    elf::ProgramHeader phdr{};
    size_t phdr_size = sizeof(elf::ProgramHeader);
    ENSURE(kernel_header.ph_count == 1);
    ENSURE(kernel_header.ph_size == phdr_size);
    EFI_CHECK(kernel_file->set_position(kernel_file, static_cast<uint64_t>(kernel_header.ph_off)));
    EFI_CHECK(kernel_file->read(kernel_file, &phdr_size, &phdr), "Failed to read kernel program header!");
    ENSURE(phdr.type == elf::SegmentType::Load);

    // Claim memory for kernel.
    const auto kernel_page_count = ustd::ceil_div(phdr.memsz, 4_KiB);
    EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAddress, efi::MemoryType::Kernel,
                                                kernel_page_count, &phdr.paddr),
              "Failed to claim physical memory for kernel!");

    // Zero out any uninitialised data.
    if (phdr.filesz != phdr.memsz) {
        ENSURE(phdr.filesz < phdr.memsz);
        ustd::fill_n(reinterpret_cast<uint8_t *>(phdr.paddr) + phdr.filesz, phdr.memsz - phdr.filesz, 0);
    }

    // Load kernel.
    EFI_CHECK(kernel_file->set_position(kernel_file, static_cast<uint64_t>(phdr.offset)));
    EFI_CHECK(kernel_file->read(kernel_file, &phdr.filesz, reinterpret_cast<void *>(phdr.paddr)),
              "Failed to read kernel!");
    EFI_CHECK(kernel_file->close(kernel_file), "Failed to close kernel file!");

    // Load all files.
    RamFsEntry *ram_fs = nullptr;
    RamFsEntry *current_entry = nullptr;
    while (traverse_directory(st, ram_fs, current_entry, root_directory, {})) {
    }
    EFI_CHECK(root_directory->close(root_directory), "Failed to close directory!");

    // Allocate stack for kernel.
    uintptr_t kernel_stack = 0;
    EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAnyPages, efi::MemoryType::LoaderData,
                                                k_kernel_stack_page_count, &kernel_stack),
              "Failed to allocate memory for kernel stack!");
    ENSURE(kernel_stack != 0, "Failed to allocate memory for kernel stack!");

    // Allocate dmesg ring buffer memory.
    const auto dmesg_area_page_count = ustd::ceil_div(sizeof(ustd::RingBuffer<char, 128_KiB>), 4_KiB);
    uintptr_t dmesg_area = 0;
    EFI_CHECK(st->boot_services->allocate_pages(efi::AllocateType::AllocateAnyPages, efi::MemoryType::Kernel,
                                                dmesg_area_page_count, &dmesg_area),
              "Failed to allocate memory for kernel dmesg area!");
    ENSURE(dmesg_area != 0, "Failed to allocate memory for kernel dmesg area!");

    // Find ACPI RSDP.
    void *rsdp = nullptr;
    st->con_out->output_string(st->con_out, L"Finding ACPI RSDP...\r\n");
    for (const auto &table : ustd::make_span(st->configuration_table, st->configuration_table_count)) {
        if (table.vendor_guid == efi::ConfigurationTable::acpi_guid) {
            rsdp = table.vendor_table;
            break;
        }
    }
    ENSURE(rsdp != nullptr, "Failed to find ACPI RSDP!");

    // Get framebuffer info.
    efi::GraphicsOutputProtocol *graphics_protocol = nullptr;
    st->con_out->output_string(st->con_out, L"Querying framebuffer info...\r\n");
    EFI_CHECK_PTR(st->boot_services->locate_protocol(&efi::GraphicsOutputProtocol::guid, nullptr,
                                                     reinterpret_cast<void **>(&graphics_protocol)),
                  graphics_protocol, "Failed to locate graphics output protocol!");

    // Find the best video mode.
    uint32_t pixel_count = 0;
    uint32_t preferred_mode = 0;
    for (uint32_t i = 0; i < graphics_protocol->mode->max_mode; i++) {
        efi::GraphicsOutputModeInfo *info = nullptr;
        size_t info_size = 0;
        EFI_CHECK(graphics_protocol->query_mode(graphics_protocol, i, &info_size, &info),
                  "Failed to query video mode!");
        if (info->width * info->height > pixel_count) {
            pixel_count = info->width * info->height;
            preferred_mode = i;
        }
    }

    // Set video mode.
    EFI_CHECK(graphics_protocol->set_mode(graphics_protocol, preferred_mode), "Failed to set video mode!");

    // Get EFI memory map data.
    size_t map_key = 0;
    size_t map_size = 0;
    size_t descriptor_size = 0;
    uint32_t descriptor_version = 0;
    st->con_out->output_string(st->con_out, L"Querying memory map size...\r\n");
    st->boot_services->get_memory_map(&map_size, nullptr, &map_key, &descriptor_size, &descriptor_version);
    ENSURE(descriptor_size >= sizeof(efi::MemoryDescriptor));

    // Allocate extra page for EFI memory map because we're about to allocate the memory for it + our memory map.
    // TODO: We should try to allocate in a loop whilst EfiStatus::BufferTooSmall is returned.
    map_size += 4096;

    // Allocate some memory for EFI memory map.
    efi::MemoryDescriptor *efi_map = nullptr;

    st->con_out->output_string(st->con_out, L"Allocating memory for EFI memory map...\r\n");
    EFI_CHECK_PTR(
        st->boot_services->allocate_pool(efi::MemoryType::LoaderData, map_size, reinterpret_cast<void **>(&efi_map)),
        efi_map, "Failed to allocate memory for EFI memory map!");

    // Allocate some memory for our memory map.
    const size_t efi_map_entry_count = map_size / descriptor_size;
    MemoryMapEntry *map = nullptr;
    st->con_out->output_string(st->con_out, L"Allocating memory for memory map...\r\n");
    EFI_CHECK_PTR(st->boot_services->allocate_pool(efi::MemoryType::LoaderData,
                                                   efi_map_entry_count * sizeof(MemoryMapEntry),
                                                   reinterpret_cast<void **>(&map)),
                  map, "Failed to allocate memory for memory map!");

    // Retrieve EFI memory map.
    st->con_out->output_string(st->con_out, L"Retrieving EFI memory map...\r\n");
    EFI_CHECK(st->boot_services->get_memory_map(&map_size, efi_map, &map_key, &descriptor_size, &descriptor_version),
              "Failed to retrieve EFI memory map!");

    // We have to be extremely careful not to call any EFI functions between now and the exit_boot_services call. This
    // means that we can't free the EFI memory map memory, but the kernel can reclaim it later.

    // Construct memory map.
    for (size_t i = 0, j = 0; i < map_size; i += descriptor_size, j++) {
        auto *descriptor = reinterpret_cast<efi::MemoryDescriptor *>(&reinterpret_cast<uint8_t *>(efi_map)[i]);
        auto *entry = &map[j];
        switch (descriptor->type) {
        case efi::MemoryType::Conventional:
            entry->type = MemoryType::Available;
            break;
        case efi::MemoryType::BootServicesCode:
        case efi::MemoryType::BootServicesData:
        case efi::MemoryType::LoaderCode:
        case efi::MemoryType::LoaderData:
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
        .dmesg_area = reinterpret_cast<void *>(dmesg_area),
        .rsdp = rsdp,
        .width = graphics_protocol->mode->info->width,
        .height = graphics_protocol->mode->info->height,
        .pixels_per_scan_line = graphics_protocol->mode->info->pixels_per_scan_line,
        .framebuffer_base = graphics_protocol->mode->framebuffer_base,
        .map = map,
        // Recalculate map entry count here. We can't use efi_map_entry_count since the memory for the memory map needs
        // to be overallocated to store an entry for itself.
        .map_entry_count = map_size / descriptor_size - 1,
        .ram_fs = ram_fs,
    };

    // Exit boot services.
    EFI_CHECK(st->boot_services->exit_boot_services(image_handle, map_key), "Failed to exit boot services!");

    // Call kernel and spin on return.
    asm volatile("mov %0, %%rsp" : : "r"(kernel_stack + k_kernel_stack_page_count * 4_KiB) : "rsp");
    reinterpret_cast<__attribute__((sysv_abi)) void (*)(BootInfo *)>(kernel_header.entry)(&boot_info);
    while (true) {
        asm volatile("cli; hlt");
    }
}
