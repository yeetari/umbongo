#include <kernel/acpi/apic_table.hh>

#include <kernel/acpi/interrupt_controller.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel::acpi {

InterruptController *ApicTable::controller(size_t index) const {
    ASSERT(index < controller_count());
    auto ptr = reinterpret_cast<uintptr_t>(m_controllers.data());
    for (size_t i = 0; i < index; i++) {
        ptr += reinterpret_cast<InterruptController *>(ptr)->length;
    }
    return reinterpret_cast<InterruptController *>(ptr);
}

size_t ApicTable::controller_count() const {
    size_t count = 0;
    auto ptr = reinterpret_cast<uintptr_t>(m_controllers.data());
    while (ptr < reinterpret_cast<uintptr_t>(this) + length()) {
        count++;
        ptr += reinterpret_cast<const InterruptController *>(ptr)->length;
    }
    return count;
}

} // namespace kernel::acpi
