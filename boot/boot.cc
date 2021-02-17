#include <boot/BootInfo.hh>
#include <boot/Efi.hh>
#include <boot/Elf.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Memory.hh>
#include <ustd/Types.hh>

namespace {

const EfiGuid g_acpi_table_guid{0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}};

const EfiGuid g_file_info_guid{0x9576e92, 0x6d3f, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};

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

    // Load loaded image protocol.
    EfiLoadedImageProtocol *loaded_image = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(image_handle, &g_loaded_image_protocol_guid,
                                                 reinterpret_cast<void **>(&loaded_image)),
              "Loaded image protocol load failed!")
    ENSURE(loaded_image != nullptr, "Loaded image protocol load failed!");

    // Load file system protcol.
    EfiSimpleFileSystemProtocol *file_system = nullptr;
    EFI_CHECK(st->boot_services->handle_protocol(loaded_image->device_handle, &g_simple_file_system_protocol_guid,
                                                 reinterpret_cast<void **>(&file_system)),
              "File system protocol load failed!")
    ENSURE(file_system != nullptr, "File system protocol load failed!");

    // Open volume.
    EfiFileProtocol *directory = nullptr;
    logln("Opening volume...");
    EFI_CHECK(file_system->open_volume(file_system, &directory), "Failed to open volume!")
    ENSURE(directory != nullptr, "Failed to open volume!");

    // Load kernel from disk.
    EfiFileProtocol *kernel_file = nullptr;
    logln("Loading kernel from disk...");
    // TODO: Don't hardcode mode&flags.
    EFI_CHECK(directory->open(directory, &kernel_file, L"kernel", 1u, 1u), "Failed to load kernel from disk!")
    ENSURE(kernel_file != nullptr, "Failed to load kernel from disk!");

    // Retrieve kernel file info.
    EfiFileInfo kernel_file_info{};
    usize kernel_file_info_size = sizeof(EfiFileInfo);
    logln("Retrieving kernel file info...");
    EFI_CHECK(kernel_file->get_info(kernel_file, &g_file_info_guid, &kernel_file_info_size, &kernel_file_info),
              "Failed to get kernel file info!")
    ENSURE(kernel_file_info.file_size != 0, "Failed to get kernel file size!");

    // Allocate memory for kernel and read kernel data.
    void *kernel_data = nullptr;
    uint64 kernel_size = kernel_file_info.file_size;
    logln("Allocating memory for kernel...");
    EFI_CHECK(st->boot_services->allocate_pool(EfiMemoryType::LoaderData, kernel_size,
                                               reinterpret_cast<void **>(&kernel_data)),
              "Failed to allocate memory for kernel!")
    logln("Reading kernel data...");
    EFI_CHECK(kernel_file->read(kernel_file, &kernel_size, kernel_data), "Failed to read kernel data!")

    // Parse kernel ELF header.
    logln("Parsing kernel ELF header...");
    auto *kernel_header = reinterpret_cast<ElfHeader *>(kernel_data);
    ENSURE(kernel_header->magic[0] == 0x7f, "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[1] == 'E', "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[2] == 'L', "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[3] == 'F', "Kernel ELF header corrupt!");

    // Parse kernel load program headers.
    uintptr kernel_entry = kernel_header->entry;
    logln("Parsing kernel load program headers...");
    for (uint16 i = 0; i < kernel_header->ph_count; i++) {
        auto *ph = reinterpret_cast<ElfProgramHeader *>(reinterpret_cast<uintptr>(kernel_data) +
                                                        static_cast<uintptr>(kernel_header->ph_off) +
                                                        kernel_header->ph_size * i);
        if (ph->type != ElfProgramHeaderType::Load) {
            continue;
        }
        const usize page_count = (ph->memsz + 4096 - 1) / 4096;
        EFI_CHECK(st->boot_services->allocate_pages(EfiAllocateType::AllocateAddress, EfiMemoryType::LoaderData,
                                                    page_count, &ph->paddr),
                  "Failed to claim physical memory for kernel!")
        // Zero out any uninitialised data.
        if (ph->filesz != ph->memsz) {
            memset(reinterpret_cast<void *>(ph->paddr), 0, ph->memsz);
        }
        kernel_file->set_position(kernel_file, static_cast<uint64>(ph->offset));
        EFI_CHECK(kernel_file->read(kernel_file, &ph->filesz, reinterpret_cast<void *>(ph->paddr)),
                  "Failed to read kernel!")
    }

    // TODO: Free memory. We could also just leave it and let the kernel reclaim it later.

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

    // Print video modes.
    for (uint32 i = 0; i < gop->mode->max_mode; i++) {
        EfiGraphicsOutputModeInfo *info = nullptr;
        usize info_size = 0;
        EFI_CHECK(gop->query_mode(gop, i, &info_size, &info), "Failed to query video mode!")
        log("{} - {}x{}, ", i, info->width, info->height);
    }

    // Ask user for preferred video mode.
    Array<unsigned char, 4> input{};
    usize input_pos = 0;
    log("\nVideo mode: ");
    while (true) {
        EfiInputKey input_key{};
        while (st->con_in->read_key_stroke(st->con_in, &input_key) == EfiStatus::NotReady) {
        }
        uint16 key = input_key.unicode_char;
        if (key == '\r' || key == '\n') {
            break;
        }
        if (key == '\t' || (key == '\b' && input_pos == 0)) {
            continue;
        }
        if (key == '\b') {
            put_char(static_cast<char>(key));
            input[input_pos--] = '\0';
            continue;
        }
        if (input_pos == input.size()) {
            continue;
        }
        put_char(static_cast<char>(key));
        input[input_pos++] = static_cast<unsigned char>(key);
    }

    uint32 preferred_mode = 0;
    for (usize i = 0; i < input.size(); i++) {
        unsigned char ch = input[i];
        if (ch == '\0') {
            continue;
        }
        uint32 digit = ch - '0';
        preferred_mode *= 10;
        preferred_mode += digit;
    }
    logln("\nSelecting mode: {}", preferred_mode);
    ENSURE(preferred_mode >= 0 && preferred_mode < gop->mode->max_mode, "Invalid mode!");
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
    };

    // Exit boot services.
    EFI_CHECK(st->boot_services->exit_boot_services(image_handle, map_key), "Failed to exit boot services!")

    // Call kernel and spin on return.
    reinterpret_cast<__attribute__((sysv_abi)) void (*)(BootInfo *)>(kernel_entry)(&boot_info);
    while (true) {
        asm volatile("cli");
        asm volatile("hlt");
    }
}
