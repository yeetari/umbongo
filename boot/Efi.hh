#pragma once

#include <ustd/Array.hh>
#include <ustd/Types.hh>

namespace efi {

struct Guid {
    uint32_t dat1;
    uint16_t dat2;
    uint16_t dat3;
    ustd::Array<uint8_t, 8> dat4;
};
using Handle = void *;

consteval size_t status_error(size_t bit) {
    return (1ull << 63ull) | bit;
}
enum class Status : size_t {
    Success = 0,
    InvalidParameter = status_error(2u),
    Unsupported = status_error(3u),
    BufferTooSmall = status_error(5u),
    NotReady = status_error(6u),
};

enum class MemoryType : uint32_t {
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

enum class MemoryFlag : uint64_t {
    Uncacheable = 1u << 0u,
    WriteCombining = 1u << 1u,
    WriteThrough = 1u << 2u,
    WriteBack = 1u << 3u,
    UncacheableExported = 1u << 4u,
    WriteProtected = 1u << 12u,
};

struct MemoryDescriptor {
    MemoryType type;
    uint32_t pad;
    uintptr_t phys_start;
    uintptr_t virt_start;
    uint64_t page_count;
    MemoryFlag flags;
};

// EFI Table Header (UEFI specification 2.8B section 4.2)
struct TableHeader {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t checksum;
    uint32_t reserved;
};

enum class AllocateType : uint32_t {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
};

// EFI Boot Services Table (UEFI specification 2.8B section 4.4)
struct BootServices : public TableHeader {
    ustd::Array<void *, 2> unused0;
    Status (*allocate_pages)(AllocateType, MemoryType, size_t page_count, uintptr_t *memory);
    void *unused1;
    Status (*get_memory_map)(size_t *size, MemoryDescriptor *map, size_t *key, size_t *descriptor_size,
                             uint32_t *descriptor_version);
    Status (*allocate_pool)(MemoryType type, size_t size, void **buffer);
    Status (*free_pool)(void *buffer);
    ustd::Array<void *, 9> unused2;
    Status (*handle_protocol)(Handle handle, const Guid *protocol, void **interface);
    ustd::Array<void *, 9> unused3;
    Status (*exit_boot_services)(Handle handle, size_t map_key);
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
    uint32_t firmware_revision;
    Handle console_in_handle;
    void *con_in;
    Handle console_out_handle;
    SimpleTextOutputProtocol *con_out;
    Handle standard_error_handle;
    void *std_err;
    void *runtime_services;
    BootServices *boot_services;
    size_t configuration_table_count;
    ConfigurationTable *configuration_table;
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
enum class FileFlag : uint64_t {
    ReadOnly = 1ull << 0u,
    Hidden = 1ull << 1u,
    System = 1ull << 2u,
    Reserved = 1ull << 3u,
    Directory = 1ull << 4u,
    Archive = 1ull << 5u,
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
enum class FileMode : uint64_t {
    Read = 1ull << 0u,
    Write = 1ull << 1u,
    Create = 1ull << 63u,
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct FileInfo {
    uint64_t size;
    uint64_t file_size;
    uint64_t physical_size;
    ustd::Array<uint8_t, 48> time;
    FileFlag flags;
    wchar_t name[1]; // NOLINT
};

// EFI File Protocol (UEFI specification 2.8B section 13.5)
struct FileProtocol {
    uint64_t revision;
    Status (*open)(FileProtocol *, FileProtocol **handle, const wchar_t *path, FileMode mode, FileFlag flags);
    Status (*close)(FileProtocol *);
    ustd::Array<void *, 1> unused0;
    Status (*read)(FileProtocol *, size_t *size, void *buffer);
    ustd::Array<void *, 2> unused1;
    Status (*set_position)(FileProtocol *, uint64_t position);
    Status (*get_info)(FileProtocol *, const Guid *info_type, size_t *size, void *buffer);
};

enum class GraphicsPixelFormat : uint32_t {
    Rgb,
    Bgr,
    Mask,
    None,
};

struct PixelMask {
    uint32_t red;
    uint32_t green;
    uint32_t blue;
    uint32_t reserved;
};

struct GraphicsOutputModeInfo {
    uint32_t version;
    uint32_t width;
    uint32_t height;
    GraphicsPixelFormat pixel_format;
    PixelMask pixel_mask;
    uint32_t pixels_per_scan_line;
};

struct GraphicsOutputProtocolMode {
    uint32_t max_mode;
    uint32_t mode;
    GraphicsOutputModeInfo *info;
    size_t size_of_info;
    uintptr_t framebuffer_base;
    size_t framebuffer_size;
};

// EFI Graphics Output Protocol (UEFI specification 2.8B section 12.9)
struct GraphicsOutputProtocol {
    static constexpr Guid guid{0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xdE, 0xd0, 0x80, 0x51, 0x6a}};
    Status (*query_mode)(GraphicsOutputProtocol *, uint32_t mode, size_t *size, GraphicsOutputModeInfo **info);
    Status (*set_mode)(GraphicsOutputProtocol *, uint32_t mode);
    void *unused0;
    GraphicsOutputProtocolMode *mode;
};

// EFI Loaded Image Protocol (UEFI specification 2.8B section 9.1)
struct LoadedImageProtocol {
    static constexpr Guid guid{0x5b1b31a1, 0x9562, 0x11d2, {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
    uint32_t revision;
    Handle parent_handle;
    SystemTable *system_table;
    Handle device_handle;
};

// EFI Simple File System Protocol (UEFI specification 2.8B section 13.4)
struct SimpleFileSystemProtocol {
    static constexpr Guid guid{0x964e5b22, 0x6459, 0x11d2, {0x8e, 0x39, 0x0, 0xa0, 0xc9, 0x69, 0x72, 0x3b}};
    uint64_t revision;
    Status (*open_volume)(SimpleFileSystemProtocol *, FileProtocol **);
};

} // namespace efi

inline constexpr efi::FileFlag operator&(efi::FileFlag a, efi::FileFlag b) {
    return static_cast<efi::FileFlag>(static_cast<size_t>(a) & static_cast<size_t>(b));
}
