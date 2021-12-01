#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace efi {

struct Guid {
    uint32 dat1;
    uint16 dat2;
    uint16 dat3;
    ustd::Array<uint8, 8> dat4;
};
using Handle = void *;

consteval usize status_error(usize bit) {
    return (1ull << 63ull) | bit;
}
enum class Status : usize {
    Success = 0,
    InvalidParameter = status_error(2u),
    Unsupported = status_error(3u),
    BufferTooSmall = status_error(5u),
    NotReady = status_error(6u),
};

enum class MemoryType : uint32 {
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

enum class MemoryFlag : uint64 {
    Uncacheable = 1u << 0u,
    WriteCombining = 1u << 1u,
    WriteThrough = 1u << 2u,
    WriteBack = 1u << 3u,
    UncacheableExported = 1u << 4u,
    WriteProtected = 1u << 12u,
};

struct MemoryDescriptor {
    MemoryType type;
    uint32 pad;
    uintptr phys_start;
    uintptr virt_start;
    uint64 page_count;
    MemoryFlag flags;
};

// EFI Table Header (UEFI specification 2.8B section 4.2)
struct TableHeader {
    uint64 signature;
    uint32 revision;
    uint32 header_size;
    uint32 checksum;
    uint32 reserved;
};

enum class AllocateType : uint32 {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
};

// EFI Boot Services Table (UEFI specification 2.8B section 4.4)
struct BootServices : public TableHeader {
    ustd::Array<void *, 2> unused0;
    Status (*allocate_pages)(AllocateType, MemoryType, usize page_count, uintptr *memory);
    void *unused1;
    Status (*get_memory_map)(usize *size, MemoryDescriptor *map, usize *key, usize *descriptor_size,
                             uint32 *descriptor_version);
    Status (*allocate_pool)(MemoryType type, usize size, void **buffer);
    Status (*free_pool)(void *buffer);
    ustd::Array<void *, 9> unused2;
    Status (*handle_protocol)(Handle handle, const Guid *protocol, void **interface);
    ustd::Array<void *, 9> unused3;
    Status (*exit_boot_services)(Handle handle, usize map_key);
    ustd::Array<void *, 10> unused4;
    Status (*locate_protocol)(const Guid *protocol, void *registration, void **interface);
};

// EFI Configuration Table (UEFI specification 2.8B section 4.6)
struct ConfigurationTable {
    static constexpr Guid acpi_guid{0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81}};
    Guid vendor_guid;
    void *vendor_table;
};

// EFI Simple Text Output Protocol (UEFI specification 2.8B section 12.4)
struct SimpleTextOutputProtocol {
    void *unused0;
    Status (*output_string)(SimpleTextOutputProtocol *, const wchar_t *);
    ustd::Array<void *, 4> unused1;
    Status (*clear_screen)(SimpleTextOutputProtocol *);
};

// EFI System Table (UEFI specification 2.8B section 4.3)
struct SystemTable : public TableHeader {
    wchar_t *firmware_vendor;
    uint32 firmware_revision;
    Handle console_in_handle;
    void *con_in;
    Handle console_out_handle;
    SimpleTextOutputProtocol *con_out;
    Handle standard_error_handle;
    void *std_err;
    void *runtime_services;
    BootServices *boot_services;
    usize configuration_table_count;
    ConfigurationTable *configuration_table;
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
enum class FileFlag : uint64 {
    ReadOnly = 1ull << 0u,
    Hidden = 1ull << 1u,
    System = 1ull << 2u,
    Reserved = 1ull << 3u,
    Directory = 1ull << 4u,
    Archive = 1ull << 5u,
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
enum class FileMode : uint64 {
    Read = 1ull << 0u,
    Write = 1ull << 1u,
    Create = 1ull << 63u,
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct FileInfo {
    uint64 size;
    uint64 file_size;
    uint64 physical_size;
    ustd::Array<uint8, 48> time;
    FileFlag flags;
    wchar_t name[1]; // NOLINT
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct FileProtocol {
    uint64 revision;
    Status (*open)(FileProtocol *, FileProtocol **handle, const wchar_t *path, FileMode mode, FileFlag flags);
    Status (*close)(FileProtocol *);
    ustd::Array<void *, 1> unused0;
    Status (*read)(FileProtocol *, usize *size, void *buffer);
    ustd::Array<void *, 2> unused1;
    Status (*set_position)(FileProtocol *, uint64 position);
    Status (*get_info)(FileProtocol *, const Guid *info_type, usize *size, void *buffer);
};

enum class GraphicsPixelFormat : uint32 {
    Rgb,
    Bgr,
    Mask,
    None,
};

struct PixelMask {
    uint32 red;
    uint32 green;
    uint32 blue;
    uint32 reserved;
};

struct GraphicsOutputModeInfo {
    uint32 version;
    uint32 width;
    uint32 height;
    GraphicsPixelFormat pixel_format;
    PixelMask pixel_mask;
    uint32 pixels_per_scan_line;
};

struct GraphicsOutputProtocolMode {
    uint32 max_mode;
    uint32 mode;
    GraphicsOutputModeInfo *info;
    usize size_of_info;
    uintptr framebuffer_base;
    usize framebuffer_size;
};

// EFI Graphics Output Protocol (UEFI specification 2.8B section 12.9)
struct GraphicsOutputProtocol {
    static constexpr Guid guid{0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xdE, 0xd0, 0x80, 0x51, 0x6a}};
    Status (*query_mode)(GraphicsOutputProtocol *, uint32 mode, usize *size, GraphicsOutputModeInfo **info);
    Status (*set_mode)(GraphicsOutputProtocol *, uint32 mode);
    void *unused0;
    GraphicsOutputProtocolMode *mode;
};

// EFI Loaded Image Protocol (UEFI specification 2.8B section 9.1)
struct LoadedImageProtocol {
    static constexpr Guid guid{0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
    uint32 revision;
    Handle parent_handle;
    SystemTable *system_table;
    Handle device_handle;
};

// EFI Simple File System Protocol (UEFI specification 2.8B section 13.4)
struct SimpleFileSystemProtocol {
    static constexpr Guid guid{0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
    uint64 revision;
    Status (*open_volume)(SimpleFileSystemProtocol *, FileProtocol **);
};

} // namespace efi

inline constexpr efi::FileFlag operator&(efi::FileFlag a, efi::FileFlag b) {
    return static_cast<efi::FileFlag>(static_cast<usize>(a) & static_cast<usize>(b));
}
