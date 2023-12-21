#include <boot/boot_info.hh>
#include <kernel/acpi/apic_table.hh>
#include <kernel/acpi/hpet_table.hh>
#include <kernel/acpi/interrupt_controller.hh>
#include <kernel/acpi/pci_table.hh>
#include <kernel/acpi/root_table.hh>
#include <kernel/acpi/root_table_ptr.hh>
#include <kernel/acpi/table.hh>
#include <kernel/console.hh>
#include <kernel/cpu/processor.hh>
#include <kernel/cpu/register_state.hh>
#include <kernel/dev/dev_fs.hh>
#include <kernel/dev/dmesg_device.hh>
#include <kernel/dev/framebuffer_device.hh>
#include <kernel/dmesg.hh>
#include <kernel/font.hh>
#include <kernel/fs/file_system.hh>
#include <kernel/fs/inode.hh>
#include <kernel/fs/ram_fs.hh>
#include <kernel/fs/vfs.hh>
#include <kernel/intr/interrupt_manager.hh>
#include <kernel/intr/interrupt_type.hh>
#include <kernel/mem/memory_manager.hh>
#include <kernel/pci/enumerate.hh>
#include <kernel/proc/scheduler.hh>
#include <kernel/proc/thread.hh>
#include <kernel/time/time_manager.hh>
#include <ustd/array.hh>
#include <ustd/assert.hh>
#include <ustd/string_view.hh>
#include <ustd/try.hh>
#include <ustd/types.hh>
#include <ustd/unique_ptr.hh>
#include <ustd/utility.hh>

#ifdef KERNEL_QEMU_DEBUG
#include <kernel/port.hh>
#endif

extern void (*k_ctors_start)();
extern void (*k_ctors_end)();

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

    // Setup in-memory file system.
    auto root_fs = ustd::make_unique<RamFs>();
    Vfs::initialise();
    Vfs::mount_root(ustd::move(root_fs));

    // Copy over files loaded by UEFI into the ramdisk.
    for (auto *entry = boot_info->ram_fs; entry != nullptr; entry = entry->next) {
        if (entry->is_directory) {
            EXPECT(Vfs::mkdir(entry->name, nullptr));
            continue;
        }
        auto *inode = EXPECT(Vfs::create(entry->name, nullptr, InodeType::RegularFile));
        inode->write({entry->data, entry->data_size}, 0);
    }

    // Create and mount the device filesystem.
    DevFs::initialise();
    DmesgDevice::initialise();

    auto *mcfg = EXPECT(xsdt->find<acpi::PciTable>());
    pci::enumerate(mcfg);

    auto *fb = new FramebufferDevice(boot_info->framebuffer_base, boot_info->width, boot_info->height,
                                     boot_info->pixels_per_scan_line * sizeof(uint32_t));
    fb->leak_ref();

    // Mark reclaimable memory as available. Note that this means boot_info is invalid to access after this point.
    MemoryManager::reclaim(boot_info);

    auto init_thread = Thread::create_user(ThreadPriority::Normal);
    EXPECT(init_thread->exec("/bin/system-server"sv));
    Scheduler::insert_thread(ustd::move(init_thread));
    Scheduler::yield_and_kill();
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
    DmesgDevice::early_initialise(boot_info);
    dmesg("core: Using font {} {}", g_font.name(), g_font.style());
#ifdef KERNEL_QEMU_DEBUG
    ENSURE(port_read(0xe9) == 0xe9, "KERNEL_QEMU_DEBUG config option enabled, but port e9 isn't available!");
#endif

    dmesg("core: boot_info = {}", boot_info);
    dmesg("core: framebuffer = {:h} ({}x{})", boot_info->framebuffer_base, boot_info->width, boot_info->height);
    dmesg("core: rsdp = {}", boot_info->rsdp);

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
    auto kernel_init_thread = Thread::create_kernel(&kernel_init, ThreadPriority::Normal);
    kernel_init_thread->register_state().rdi = reinterpret_cast<uintptr_t>(boot_info);
    kernel_init_thread->register_state().rsi = reinterpret_cast<uintptr_t>(xsdt);

    Scheduler::initialise();
    Scheduler::insert_thread(ustd::move(kernel_init_thread));
    Scheduler::start();
}

} // namespace kernel
