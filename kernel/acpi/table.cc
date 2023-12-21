#include <kernel/acpi/table.hh>

#include <ustd/types.hh>

namespace kernel::acpi {

bool Table::valid() const {
    const auto *const dat = reinterpret_cast<const uint8_t *>(this);
    uint8_t sum = 0;
    for (const auto *ptr = dat; ptr < dat + m_length; ptr++) {
        sum += *ptr;
    }
    return sum == 0;
}

} // namespace kernel::acpi
