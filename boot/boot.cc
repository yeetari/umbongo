#include <boot/BootInfo.hh>
#include <boot/Efi.hh>
#include <boot/Elf.hh>
#include <ustd/Types.hh>

namespace {

const EfiGuid g_file_info_guid{0x9576E92, 0x6D3F, 0x11D2, {0x8E, 0x39, 0x0, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

const EfiGuid g_graphics_output_protocol_guid{
    0x9042A9DE, 0x23DC, 0x4A38, {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A}};

const EfiGuid g_loaded_image_protocol_guid{
    0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};

const EfiGuid g_simple_file_system_protocol_guid{
    0x964E5B22, 0x6459, 0x11D2, {0x8E, 0x39, 0x0, 0xA0, 0xC9, 0x69, 0x72, 0x3B}};
} // namespace

// TODO: Remove this.
extern "C" void *memset(void *bufptr, int value, usize size) {
    auto *buf = static_cast<unsigned char *>(bufptr);
    for (usize i = 0; i < size; i++) {
        buf[i] = static_cast<unsigned char>(value);
    }
    return bufptr;
}

EfiStatus efi_main(EfiHandle image_handle, EfiSystemTable *st) {
    // Flush input buffer and clear screen.
    st->con_in->reset(st->con_in, false);
    st->con_out->clear_screen(st->con_out);

    // Load loaded image protocol.
    EfiLoadedImageProtocol *loaded_image = nullptr;
    {
        EfiStatus status = st->boot_services->handle_protocol(image_handle, &g_loaded_image_protocol_guid,
                                                              reinterpret_cast<void **>(&loaded_image));
        if (status != EfiStatus::Success || loaded_image == nullptr) {
            st->con_out->output_string(st->con_out, L"Loaded image protocol failed!\r\n");
            while (true) {
            }
        }
    }

    // Mount file system.
    EfiSimpleFileSystemProtocol *file_system = nullptr;
    {
        EfiStatus status = st->boot_services->handle_protocol(
            loaded_image->device_handle, &g_simple_file_system_protocol_guid, reinterpret_cast<void **>(&file_system));
        if (status != EfiStatus::Success || file_system == nullptr) {
            st->con_out->output_string(st->con_out, L"File system protocol load failed!\r\n");
            while (true) {
            }
        }
    }

    // Open volume.
    EfiFileProtocol *directory = nullptr;
    st->con_out->output_string(st->con_out, L"Opening volume...\r\n");
    {
        EfiStatus status = file_system->open_volume(file_system, &directory);
        if (status != EfiStatus::Success || directory == nullptr) {
            st->con_out->output_string(st->con_out, L"Failed to open volume!\r\n");
            while (true) {
            }
        }
    }

    // Load kernel from disk.
    EfiFileProtocol *kernel_file = nullptr;
    st->con_out->output_string(st->con_out, L"Loading kernel from disk...\r\n");
    {
        // TODO: Don't hardcode mode&flags.
        EfiStatus status = directory->open(directory, &kernel_file, L"kernel", 1U, 1U);
        if (status != EfiStatus::Success || kernel_file == nullptr) {
            st->con_out->output_string(st->con_out, L"Failed to load kernel from disk!\r\n");
            while (true) {
            }
        }
    }

    // Allocate memory for kernel.
    void *kernel_data = nullptr;
    st->con_out->output_string(st->con_out, L"Allocating memory for kernel...\r\n");
    {
        EfiFileInfo file_info{};
        usize file_info_size = sizeof(EfiFileInfo);
        EfiStatus status = kernel_file->get_info(kernel_file, &g_file_info_guid, &file_info_size, &file_info);
        if (status != EfiStatus::Success || file_info.file_size == 0) {
            st->con_out->output_string(st->con_out, L"Failed to get kernel file info size!\r\n");
            while (true) {
            }
        }

        uint64 kernel_size = file_info.file_size;
        status = st->boot_services->allocate_pool(EfiMemoryType::LoaderData, kernel_size,
                                                  reinterpret_cast<void **>(&kernel_data));
        if (status != EfiStatus::Success) {
            st->con_out->output_string(st->con_out, L"Failed to allocate memory for kernel!\r\n");
            while (true) {
            }
        }

        status = kernel_file->read(kernel_file, &kernel_size, kernel_data);
        if (status != EfiStatus::Success) {
            st->con_out->output_string(st->con_out, L"Failed to copy kernel data!\r\n");
            while (true) {
            }
        }
    }

    // Parse kernel ELF.
    uintptr kernel_entry = 0;
    st->con_out->output_string(st->con_out, L"Parsing kernel ELF...\r\n");
    {
        auto *header = reinterpret_cast<ElfHeader *>(kernel_data);
        if (header->magic[0] != 0x7F || header->magic[1] != 'E' || header->magic[2] != 'L' || header->magic[3] != 'F') {
            st->con_out->output_string(st->con_out, L"Kernel corrupt!\r\n");
            while (true) {
            }
        }

        kernel_entry = header->entry;
        for (uint16 i = 0; i < header->ph_count; i++) {
            auto *ph = reinterpret_cast<ElfProgramHeader *>(reinterpret_cast<uintptr>(kernel_data) +
                                                            static_cast<uintptr>(header->ph_off) + header->ph_size * i);
            if (ph->type != ElfProgramHeaderType::Load) {
                continue;
            }
            const usize page_count = (ph->memsz + 4096 - 1) / 4096;
            uintptr paddr = ph->paddr;
            EfiStatus status = st->boot_services->allocate_pages(EfiAllocateType::AllocateAddress,
                                                                 EfiMemoryType::LoaderData, page_count, &paddr);
            if (status != EfiStatus::Success) {
                st->con_out->output_string(
                    st->con_out, L"Failed to claim physical memory for kernel, memory must already be in use!\r\n");
                while (true) {
                }
            }
            usize size = ph->filesz;
            kernel_file->set_position(kernel_file, static_cast<uint64>(ph->offset));
            status = kernel_file->read(kernel_file, &size, reinterpret_cast<void *>(paddr));
            if (status != EfiStatus::Success) {
                st->con_out->output_string(st->con_out, L"Failed to copy kernel!\r\n");
                while (true) {
                }
            }
        }
    }

    // TODO: Free memory.

    // Get framebuffer info.
    EfiGraphicsOutputProtocol *gop = nullptr;
    st->con_out->output_string(st->con_out, L"Querying framebuffer info...\r\n");
    {
        EfiStatus status = st->boot_services->locate_protocol(&g_graphics_output_protocol_guid, nullptr,
                                                              reinterpret_cast<void **>(&gop));
        if (status != EfiStatus::Success || gop == nullptr) {
            st->con_out->output_string(st->con_out, L"Failed to locate graphics output protocol!\r\n");
            while (true) {
            }
        }
    }

    // Get memory map data.
    usize map_key = 0;
    usize map_size = 0;
    usize descriptor_size = 0;
    uint32 descriptor_version = 0;
    st->con_out->output_string(st->con_out, L"Querying memory map size...\r\n");
    {
        EfiStatus status =
            st->boot_services->get_memory_map(&map_size, nullptr, &map_key, &descriptor_size, &descriptor_version);
        if (status == EfiStatus::InvalidParameter) {
            st->con_out->output_string(st->con_out, L"Failed to query memory map size!\r\n");
            while (true) {
            }
        }
    }

    // Allocate extra page for memory map.
    map_size += 4096;

    // Allocate some memory for memory map.
    EfiMemoryDescriptor *map = nullptr;
    st->con_out->output_string(st->con_out, L"Allocating memory for memory map...\r\n");
    {
        EfiStatus status =
            st->boot_services->allocate_pool(EfiMemoryType::LoaderData, map_size, reinterpret_cast<void **>(&map));
        if (status != EfiStatus::Success || map == nullptr) {
            st->con_out->output_string(st->con_out, L"Failed to allocate memory for memory map!\r\n");
            while (true) {
            }
        }
    }

    // Copy memory map.
    st->con_out->output_string(st->con_out, L"Copying memory map...\r\n");
    {
        EfiStatus status =
            st->boot_services->get_memory_map(&map_size, map, &map_key, &descriptor_size, &descriptor_version);
        if (status != EfiStatus::Success) {
            st->con_out->output_string(st->con_out, L"Failed to copy memory map!\r\n");
            while (true) {
            }
        }
    }

    // Fill out boot info.
    BootInfo boot_info{
        .width = gop->mode->info->width,
        .height = gop->mode->info->height,
        .pixels_per_scan_line = gop->mode->info->pixels_per_scan_line,
        .framebuffer_base = gop->mode->framebuffer_base,
    };

    // Exit boot services.
    if (st->boot_services->exit_boot_services(image_handle, map_key) != EfiStatus::Success) {
        st->con_out->output_string(st->con_out, L"Failed to exit boot services!\r\n");
        while (true) {
        }
    }

    // Call kernel and spin on return.
    reinterpret_cast<__attribute__((sysv_abi)) void (*)(BootInfo *)>(kernel_entry)(&boot_info);
    while (true) {
    }
}
