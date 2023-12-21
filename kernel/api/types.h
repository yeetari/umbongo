#pragma once

#include <ustd/int_types.h>

typedef struct ub_fd_pair {
    uint32_t parent;
    uint32_t child;
} ub_fd_pair_t;

typedef struct ub_fb_info {
    size_t size;
    uint32_t width;
    uint32_t height;
} ub_fb_info_t;

typedef struct ub_pci_bar {
    uintptr_t address;
    size_t size;
} ub_pci_bar_t;

typedef struct ub_pci_device_info {
    ub_pci_bar_t bars[6]; // NOLINT
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t clas;
    uint8_t subc;
    uint8_t prif;
} ub_pci_device_info_t;

typedef enum ub_poll_events {
    UB_POLL_EVENT_READ = 1u << 0u,
    UB_POLL_EVENT_WRITE = 1u << 1u,
    UB_POLL_EVENT_ACCEPT = UB_POLL_EVENT_READ,
} ub_poll_events_t;

typedef struct ub_poll_fd {
    uint32_t fd;
    ub_poll_events_t events;
    ub_poll_events_t revents;
} ub_poll_fd_t;

typedef enum ub_ioctl_request {
    UB_IOCTL_REQUEST_FB_GET_INFO,
    UB_IOCTL_REQUEST_PCI_ENABLE_DEVICE,
    UB_IOCTL_REQUEST_PCI_ENABLE_INTERRUPTS,
} ub_ioctl_request_t;

typedef enum ub_memory_prot {
    UB_MEMORY_PROT_WRITE = 1u << 0u,
    UB_MEMORY_PROT_EXEC = 1u << 1u,
    UB_MEMORY_PROT_UNCACHEABLE = 1u << 2u,
} ub_memory_prot_t;

typedef enum ub_open_mode {
    UB_OPEN_MODE_NONE = 0,
    UB_OPEN_MODE_CREATE = 1u << 0u,
    UB_OPEN_MODE_TRUNCATE = 1u << 1u,
} ub_open_mode_t;

typedef enum ub_seek_mode {
    UB_SEEK_MODE_ADD,
    UB_SEEK_MODE_SET,
} ub_seek_mode_t;

#ifdef __cplusplus

inline constexpr ub_poll_events_t operator|(ub_poll_events_t lhs, ub_poll_events_t rhs) {
    return static_cast<ub_poll_events_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

inline constexpr ub_open_mode_t operator|(ub_open_mode_t lhs, ub_open_mode_t rhs) {
    return static_cast<ub_open_mode_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

#endif
