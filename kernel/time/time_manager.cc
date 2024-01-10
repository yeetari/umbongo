#include <kernel/time/time_manager.hh>

#include <kernel/acpi/generic_address.hh>
#include <kernel/acpi/hpet_table.hh>
#include <kernel/time/hpet.hh>
#include <ustd/assert.hh>
#include <ustd/types.hh>

namespace kernel {
namespace {

Hpet *s_hpet;
uint64_t s_ns_since_boot = 0;

} // namespace

void TimeManager::initialise(const acpi::HpetTable *hpet_table) {
    // Retrieve HPET address from ACPI tables and ensure the values are sane.
    const auto &hpet_address = hpet_table->base_address();
    ENSURE(hpet_address.address_space == acpi::AddressSpace::SystemMemory);
    ENSURE(hpet_address.register_bit_offset == 0);

    // Enable the main counter.
    s_hpet = new Hpet(hpet_address.address);
    s_hpet->enable();
}

void TimeManager::spin(uint64_t millis) {
    s_hpet->spin(millis);
}

void TimeManager::update() {
    s_ns_since_boot += s_hpet->update_time();
}

uint64_t TimeManager::ns_since_boot() {
    return s_ns_since_boot;
}

} // namespace kernel
