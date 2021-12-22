#include <kernel/acpi/ApicTable.hh>

#include <kernel/acpi/InterruptController.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/Types.hh>

namespace kernel::acpi {

InterruptController *ApicTable::controller(usize index) const {
    ASSERT(index < controller_count());
    auto ptr = reinterpret_cast<uintptr>(m_controllers.data());
    for (usize i = 0; i < index; i++) {
        ptr += reinterpret_cast<InterruptController *>(ptr)->length;
    }
    return reinterpret_cast<InterruptController *>(ptr);
}

usize ApicTable::controller_count() const {
    usize count = 0;
    auto ptr = reinterpret_cast<uintptr>(m_controllers.data());
    while (ptr < reinterpret_cast<uintptr>(this) + length()) {
        count++;
        ptr += reinterpret_cast<const InterruptController *>(ptr)->length;
    }
    return count;
}

} // namespace kernel::acpi
