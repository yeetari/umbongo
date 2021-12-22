#pragma once

#include <ustd/Types.hh>

namespace kernel::acpi {

enum class AccessSize : uint8 {
    Byte = 1,
    Word = 2,
    Dword = 3,
    Qword = 4,
};

enum class AddressSpace : uint8 {
    SystemMemory = 0,
    SystemIo = 1,
    PciConfiguration = 2,
};

struct [[gnu::packed]] GenericAddress {
    AddressSpace address_space;
    uint8 register_bit_width;
    uint8 register_bit_offset;
    AccessSize access_size;
    uint64 address;
};

} // namespace kernel::acpi
