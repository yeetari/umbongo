#include <kernel/acpi/Table.hh>

#include <ustd/Types.hh>

namespace kernel::acpi {

bool Table::valid() const {
    const auto *const dat = reinterpret_cast<const uint8 *>(this);
    uint8 sum = 0;
    for (const auto *ptr = dat; ptr < dat + m_length; ptr++) {
        sum += *ptr;
    }
    return sum == 0;
}

} // namespace kernel::acpi
