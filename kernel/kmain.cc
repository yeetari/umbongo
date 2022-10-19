#include <boot/BootInfo.hh>
#include <kernel/Config.hh>
#include <kernel/Console.hh>
#include <kernel/Dmesg.hh>
#include <kernel/Font.hh>
#include <kernel/Port.hh>
#include <kernel/acpi/ApicTable.hh>
#include <kernel/acpi/HpetTable.hh>
#include <kernel/acpi/InterruptController.hh>
#include <kernel/acpi/PciTable.hh>
#include <kernel/acpi/RootTable.hh>
#include <kernel/acpi/RootTablePtr.hh>
#include <kernel/acpi/Table.hh>
#include <kernel/api/BootFs.hh>
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/intr/InterruptType.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/mem/Region.hh>
#include <kernel/mem/VirtSpace.hh>
#include <kernel/mem/VmObject.hh>
#include <kernel/proc/Process.hh>
#include <kernel/proc/Scheduler.hh>
#include <kernel/proc/Thread.hh>
#include <kernel/proc/ThreadPriority.hh>
#include <kernel/time/TimeManager.hh>
#include <ustd/Array.hh>
#include <ustd/Assert.hh>
#include <ustd/StringView.hh>
#include <ustd/Try.hh>
#include <ustd/Types.hh>
#include <ustd/UniquePtr.hh>
#include <ustd/Utility.hh>

extern void (*k_ctors_start)();
extern void (*k_ctors_end)();

// TODO: Randomise value.
size_t __stack_chk_guard = 0xdeadc0de;

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    kernel::dmesg_no_lock("\nAssertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        kernel::dmesg_no_lock("=> {}", msg);
    }
    while (true) {
        asm volatile("cli; hlt");
    }
}

namespace kernel {

class LocalApic;

namespace {

InterruptPolarity interrupt_polarity(uint8_t polarity) {
    switch (polarity) {
    case 0b00:
    case 0b01:
        return InterruptPolarity::ActiveHigh;
    case 0b11:
        return InterruptPolarity::ActiveLow;
    default:
        ENSURE_NOT_REACHED();
    }
}

InterruptTriggerMode interrupt_trigger_mode(uint8_t trigger_mode) {
    switch (trigger_mode) {
    case 0b00:
    case 0b01:
        return InterruptTriggerMode::EdgeTriggered;
    case 0b11:
        return InterruptTriggerMode::LevelTriggered;
    default:
        ENSURE_NOT_REACHED();
    }
}

void kernel_init(BootInfo *boot_info, acpi::RootTable *xsdt) {
    auto *madt = EXPECT(xsdt->find<acpi::ApicTable>());
    Processor::start_aps(madt);

    size_t boot_fs_size = sizeof(BootFsEntry);
    for (auto *entry = boot_info->initramfs; entry != nullptr; entry = entry->next) {
        if (entry->is_directory) {
            continue;
        }
        boot_fs_size += sizeof(BootFsEntry) + entry->name_length + entry->data_size;
    }

    auto boot_fs_vmo = VmObject::create(boot_fs_size);
    auto copy_region = EXPECT(MemoryManager::current_space()->allocate_anywhere(RegionAccess::Writable, boot_fs_vmo));
    auto *copy_base = reinterpret_cast<uint8_t *>(copy_region->base());
    for (auto *entry = boot_info->initramfs; entry != nullptr; entry = entry->next) {
        if (entry->is_directory) {
            continue;
        }
        BootFsEntry new_entry(entry->data_size, entry->name_length);
        __builtin_memcpy(copy_base, &new_entry, sizeof(BootFsEntry));
        copy_base += sizeof(BootFsEntry);

        __builtin_memcpy(copy_base, entry + 1, entry->name_length);
        copy_base += entry->name_length;

        if (entry->data_size != 0) {
            __builtin_memcpy(copy_base, reinterpret_cast<uint8_t *>(entry + 1) + entry->name_length, entry->data_size);
        }
        copy_base += entry->data_size;
    }

    BootFsEntry terminator_entry(size_t(-1), 0);
    __builtin_memcpy(copy_base, &terminator_entry, sizeof(BootFsEntry));
    ENSURE(copy_base + sizeof(BootFsEntry) == reinterpret_cast<uint8_t *>(copy_region->base() + boot_fs_size));

    // Mark reclaimable memory as available. Note that this means boot_info is invalid to access after this point.
    MemoryManager::reclaim(boot_info);

    const auto &root_server_entry =
        EXPECT(reinterpret_cast<BootFsEntry *>(copy_region->base())->find("/bin/root-server"sv));
    const auto root_server_data = root_server_entry.data();
    const auto root_server_ptr = UserPtr<const uint8_t>::from_raw(root_server_data.data());

    auto root_thread = Thread::create_user(ThreadPriority::Normal, "/bin/root-server");
    ENSURE(root_thread->process().allocate_handle(boot_fs_vmo) == 0);
    EXPECT(root_thread->exec(root_server_ptr, root_server_data.size()));
    copy_region.clear();

    Scheduler::insert_thread(ustd::move(root_thread));
    Thread::kill_and_yield();
}

} // namespace

extern "C" [[noreturn]] void __stack_chk_fail() {
    auto *rbp = static_cast<uint64_t *>(__builtin_frame_address(0));
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64_t *>(*rbp);
    }
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

extern "C" void kmain(BootInfo *boot_info) {
    Console::initialise(boot_info);
    dmesg("core: boot_info = {}", boot_info);
    dmesg("core: framebuffer = {:h} ({}x{})", boot_info->framebuffer_base, boot_info->width, boot_info->height);
    dmesg("core: rsdp = {}", boot_info->rsdp);
    if constexpr (k_kernel_stack_protector) {
        dmesg("core: Stack protector initialised");
    }

    MemoryManager::initialise(boot_info);
    Processor::initialise();
    Processor::setup(0);

    // Invoke global constructors.
    dmesg("core: Invoking {} global constructors", &k_ctors_end - &k_ctors_start);
    for (void (**ctor)() = &k_ctors_start; ctor < &k_ctors_end; ctor++) {
        (*ctor)();
    }

    auto *rsdp = reinterpret_cast<acpi::RootTablePtr *>(boot_info->rsdp);
    ENSURE(rsdp->revision() == 2, "ACPI 2.0+ required!");

    auto *xsdt = rsdp->xsdt();
    dmesg("acpi: XSDT = {} (revision={}, valid={})", xsdt, xsdt->revision(), xsdt->valid());
    ENSURE(xsdt->valid());

    for (auto *entry : *xsdt) {
        dmesg("acpi:  - {:c}{:c}{:c}{:c} = {} (revision={}, valid={})", entry->signature()[0], entry->signature()[1],
              entry->signature()[2], entry->signature()[3], entry, entry->revision(), entry->valid());
    }

    auto *madt = EXPECT(xsdt->find<acpi::ApicTable>());
    if ((madt->flags() & 1u) != 0) {
        InterruptManager::mask_pic();
    }

    for (auto *controller : *madt) {
        // TODO: Handle local APIC address override.
        ENSURE(controller->type != 5);
        if (controller->type == 1) {
            auto *io_apic = reinterpret_cast<acpi::IoApicController *>(controller);
            InterruptManager::register_io_apic(io_apic->address, io_apic->gsi_base);
        }
        if (controller->type == 2) {
            auto *override = reinterpret_cast<acpi::InterruptSourceOverride *>(controller);
            ENSURE(override->bus == 0);
            InterruptManager::register_override(override->isa, override->gsi, interrupt_polarity(override->polarity),
                                                interrupt_trigger_mode(override->trigger_mode));
        }
    }

    auto *apic = reinterpret_cast<LocalApic *>(madt->local_apic());
    dmesg("acpi: Local APIC = {}", apic);
    Processor::set_apic(apic);

    auto *hpet_table = EXPECT(xsdt->find<acpi::HpetTable>());
    TimeManager::initialise(hpet_table);

    // Start a new kernel thread that will perform the rest of the initialisation. We do this so that we can safely
    // start kernel threads, and so that the current stack we are using is no longer in use, meaning we can reclaim the
    // memory.
    auto kernel_init_thread = Thread::create_kernel(&kernel_init, ThreadPriority::Normal, {});
    kernel_init_thread->register_state().rdi = reinterpret_cast<uintptr_t>(boot_info);
    kernel_init_thread->register_state().rsi = reinterpret_cast<uintptr_t>(xsdt);

    Scheduler::initialise();
    Scheduler::insert_thread(ustd::move(kernel_init_thread));
    Scheduler::start();
}

} // namespace kernel
