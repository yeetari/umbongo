#pragma once

#include <ustd/types.hh>

namespace kernel::acpi {

enum class AccessSize : uint8_t {
    Byte = 1,
    Word = 2,
    Dword = 3,
    Qword = 4,
};

enum class AddressSpace : uint8_t {
    SystemMemory = 0,
    SystemIo = 1,
    PciConfiguration = 2,
};

struct [[gnu::packed]] GenericAddress {
    AddressSpace address_space;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    AccessSize access_size;
    uint64_t address;
};

} // namespace kernel::acpi
