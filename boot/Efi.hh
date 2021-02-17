#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

using EfiHandle = void *;

#define EFI_ERR(x) ((static_cast<usize>(1u) << 63u) | (x))
enum class EfiStatus : usize {
    Success = 0,
    InvalidParameter = EFI_ERR(2u),
    Unsupported = EFI_ERR(3u),
    BufferTooSmall = EFI_ERR(5u),
    NotReady = EFI_ERR(6u),
};

struct EfiInputKey {
    uint16 scan_code;
    uint16 unicode_char;
};

struct EfiGuid {
    uint32 dat1;
    uint16 dat2;
    uint16 dat3;
    Array<uint8, 8> dat4;
};

// EFI Table Header (UEFI specification 2.8B section 4.2)
struct EfiTableHeader {
    uint64 signature;
    uint32 revision;
    uint32 header_size;
    uint32 checksum;
    uint32 reserved;
};

// EFI Simple Text Input Protocol (UEFI specification 2.8B section 12.3)
struct EfiSimpleTextInputProtocol {
    EfiStatus (*reset)(EfiSimpleTextInputProtocol *, bool);
    EfiStatus (*read_key_stroke)(EfiSimpleTextInputProtocol *, EfiInputKey *key);
};

// EFI Simple Text Output Protocol (UEFI specification 2.8B section 12.4)
struct EfiSimpleTextOutputProtocol {
    void *reset;
    EfiStatus (*output_string)(EfiSimpleTextOutputProtocol *, const wchar_t *);
    void *test_string;
    void *query_mode;
    void *set_mode;
    void *set_attribute;
    EfiStatus (*clear_screen)(EfiSimpleTextOutputProtocol *);
    void *set_cursor_position;
    void *enable_cursor;
    void *mode;
};

enum class EfiAllocateType : uint32 {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
};

enum class EfiMemoryType : uint32 {
    Reserved,
    LoaderCode,
    LoaderData,
    BootServicesCode,
    BootServicesData,
    RuntimeServicesCode,
    RuntimeServicesData,
    Conventional,
    Unusable,
    AcpiReclaimable,
    AcpiNvs,
    MemoryMappedIo,
    MemoryMappedIoPortSpace,
    PalCode,
    PersistentMemory,
};

enum class EfiMemoryFlag : uint64 {
    Uncacheable = 1u << 0u,
    WriteCombining = 1u << 1u,
    WriteThrough = 1u << 2u,
    WriteBack = 1u << 3u,
    UncacheableExported = 1u << 4u,
    WriteProtected = 1u << 12u,
};

struct EfiMemoryDescriptor {
    EfiMemoryType type;
    uint32 pad;
    uintptr phys_start;
    uintptr virt_start;
    uint64 page_count;
    EfiMemoryFlag flags;
};

// EFI Boot Services Table (UEFI specification 2.8B section 4.4)
struct EfiBootServicesTable : public EfiTableHeader {
    Array<void *, 2> unused0;
    EfiStatus (*allocate_pages)(EfiAllocateType, EfiMemoryType, usize page_count, uintptr *memory);
    void *unused1;
    EfiStatus (*get_memory_map)(usize *size, EfiMemoryDescriptor *map, usize *key, usize *descriptor_size,
                                uint32 *descriptor_version);
    EfiStatus (*allocate_pool)(EfiMemoryType type, usize size, void **buffer);
    EfiStatus (*free_pool)(void *buffer);
    Array<void *, 9> unused2;
    EfiStatus (*handle_protocol)(EfiHandle handle, const EfiGuid *protocol, void **interface);
    Array<void *, 9> unused3;
    EfiStatus (*exit_boot_services)(EfiHandle handle, usize map_key);
    Array<void *, 10> unused4;
    EfiStatus (*locate_protocol)(const EfiGuid *protocol, void *registration, void **interface);
};

// EFI System Table (UEFI specification 2.8B section 4.3)
struct EfiSystemTable : public EfiTableHeader {
    wchar_t *firmware_vendor;
    uint32 firmware_revision;
    EfiHandle console_in_handle;
    EfiSimpleTextInputProtocol *con_in;
    EfiHandle console_out_handle;
    EfiSimpleTextOutputProtocol *con_out;
    EfiHandle standard_error_handle;
    void *std_err;
    void *runtime_services;
    EfiBootServicesTable *boot_services;
    usize table_entry_count;
    void *configuration_table;
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct EfiFileInfo {
    uint64 size;
    uint64 file_size;
    uint64 physical_size;
    Array<uint8, 66> junk;
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct EfiFileProtocol {
    uint64 revision;
    EfiStatus (*open)(EfiFileProtocol *, EfiFileProtocol **handle, const wchar_t *path, uint64 mode, uint64 flags);
    Array<void *, 2> unused0;
    EfiStatus (*read)(EfiFileProtocol *, usize *size, void *buffer);
    Array<void *, 2> unused1;
    EfiStatus (*set_position)(EfiFileProtocol *, uint64 position);
    EfiStatus (*get_info)(EfiFileProtocol *, const EfiGuid *info_type, usize *size, void *buffer);
};

struct EfiPixelMask {
    uint32 red;
    uint32 green;
    uint32 blue;
    uint32 reserved;
};

enum class EfiGraphicsPixelFormat : uint32 {
    Rgb,
    Bgr,
    Mask,
    None,
};

struct EfiGraphicsOutputModeInfo {
    uint32 version;
    uint32 width;
    uint32 height;
    EfiGraphicsPixelFormat pixel_format;
    EfiPixelMask pixel_mask;
    uint32 pixels_per_scan_line;
};

struct EfiGraphicsOutputProtocolMode {
    uint32 max_mode;
    uint32 mode;
    EfiGraphicsOutputModeInfo *info;
    usize size_of_info;
    uintptr framebuffer_base;
    usize framebuffer_size;
};

// EFI Graphics Output Protocol (UEFI specification 2.8B section 12.9)
struct EfiGraphicsOutputProtocol {
    void *unused0;
    EfiStatus (*set_mode)(EfiGraphicsOutputProtocol *, uint32 mode);
    void *unused1;
    EfiGraphicsOutputProtocolMode *mode;
};

// EFI Loaded Image Protocol (UEFI specification 2.8B section 9.1)
struct EfiLoadedImageProtocol {
    uint32 revision;
    EfiHandle parent_handle;
    EfiSystemTable *system_table;
    EfiHandle device_handle;
};

// EFI Simple File System Protocol (UEFI specification 2.8B section 13.4)
struct EfiSimpleFileSystemProtocol {
    uint64 revision;
    EfiStatus (*open_volume)(EfiSimpleFileSystemProtocol *, EfiFileProtocol **);
};
