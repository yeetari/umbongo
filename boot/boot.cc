#include <boot/BootInfo.hh>
#include <boot/Efi.hh>
#include <boot/Elf.hh>
#include <ustd/Assert.hh>
#include <ustd/Log.hh>
#include <ustd/Types.hh>

namespace {

const EfiGuid g_file_info_guid{0x9576E92, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x0, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

const EfiGuid g_graphics_output_protocol_guid{
    0x9042A9DE, 0x23DC, 0x4A38, {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A}};

const EfiGuid g_loaded_image_protocol_guid{
    0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

const EfiGuid g_simple_file_system_protocol_guid{
    0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x0, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

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
    log("Opening volume...");
    EFI_CHECK(file_system->open_volume(file_system, &directory), "Failed to open volume!")
    ENSURE(directory != nullptr, "Failed to open volume!");

    // Load kernel from disk.
    EfiFileProtocol *kernel_file = nullptr;
    log("Loading kernel from disk...");
    // TODO: Don't hardcode mode&flags.
    EFI_CHECK(directory->open(directory, &kernel_file, L"kernel", 1U, 1U), "Failed to load kernel from disk!")
    ENSURE(kernel_file != nullptr, "Failed to load kernel from disk!");

    // Retrieve kernel file info.
    EfiFileInfo kernel_file_info{};
    usize kernel_file_info_size = sizeof(EfiFileInfo);
    log("Retrieving kernel file info...");
    EFI_CHECK(kernel_file->get_info(kernel_file, &g_file_info_guid, &kernel_file_info_size, &kernel_file_info),
              "Failed to get kernel file info!")
    ENSURE(kernel_file_info.file_size != 0, "Failed to get kernel file size!");

    // Allocate memory for kernel and read kernel data.
    void *kernel_data = nullptr;
    uint64 kernel_size = kernel_file_info.file_size;
    log("Allocating memory for kernel...");
    EFI_CHECK(st->boot_services->allocate_pool(EfiMemoryType::LoaderData, kernel_size,
                                               reinterpret_cast<void **>(&kernel_data)),
              "Failed to allocate memory for kernel!")
    log("Reading kernel data...");
    EFI_CHECK(kernel_file->read(kernel_file, &kernel_size, kernel_data), "Failed to read kernel data!")

    // Parse kernel ELF header.
    log("Parsing kernel ELF header...");
    auto *kernel_header = reinterpret_cast<ElfHeader *>(kernel_data);
    ENSURE(kernel_header->magic[0] == 0x7F, "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[1] == 'E', "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[2] == 'L', "Kernel ELF header corrupt!");
    ENSURE(kernel_header->magic[3] == 'F', "Kernel ELF header corrupt!");

    // Parse kernel load program headers.
    uintptr kernel_entry = kernel_header->entry;
    log("Parsing kernel load program headers...");
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
        kernel_file->set_position(kernel_file, static_cast<uint64>(ph->offset));
        EFI_CHECK(kernel_file->read(kernel_file, &ph->filesz, reinterpret_cast<void *>(ph->paddr)),
                  "Failed to read kernel!")
    }

    // TODO: Free memory.

    // Get framebuffer info.
    EfiGraphicsOutputProtocol *gop = nullptr;
    log("Querying framebuffer info...");
    EFI_CHECK(
        st->boot_services->locate_protocol(&g_graphics_output_protocol_guid, nullptr, reinterpret_cast<void **>(&gop)),
        "Failed to locate graphics output protocol!")
    ENSURE(gop != nullptr, "Failed to locate graphics output protocol!");

    // Get memory map data.
    usize map_key = 0;
    usize map_size = 0;
    usize descriptor_size = 0;
    uint32 descriptor_version = 0;
    log("Querying memory map size...");
    st->boot_services->get_memory_map(&map_size, nullptr, &map_key, &descriptor_size, &descriptor_version);

    // Allocate extra page for memory map.
    map_size += 4096;

    // Allocate some memory for memory map.
    EfiMemoryDescriptor *map = nullptr;
    log("Allocating memory for memory map...");
    EFI_CHECK(st->boot_services->allocate_pool(EfiMemoryType::LoaderData, map_size, reinterpret_cast<void **>(&map)),
              "Failed to allocate memory for memory map!")
    ENSURE(map != nullptr, "Failed to allocate memory for memory map!");

    // Copy memory map.
    log("Copying memory map...");
    EFI_CHECK(st->boot_services->get_memory_map(&map_size, map, &map_key, &descriptor_size, &descriptor_version),
              "Failed to copy memory map!")

    // Fill out boot info.
    BootInfo boot_info{
        .width = gop->mode->info->width,
        .height = gop->mode->info->height,
        .pixels_per_scan_line = gop->mode->info->pixels_per_scan_line,
        .framebuffer_base = gop->mode->framebuffer_base,
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
