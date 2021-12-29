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
#include <kernel/cpu/Processor.hh>
#include <kernel/cpu/RegisterState.hh>
#include <kernel/dev/DevFs.hh>
#include <kernel/dev/DmesgDevice.hh>
#include <kernel/dev/FramebufferDevice.hh>
#include <kernel/fs/FileSystem.hh>
#include <kernel/fs/Inode.hh>
#include <kernel/fs/InodeType.hh>
#include <kernel/fs/RamFs.hh>
#include <kernel/fs/Vfs.hh>
#include <kernel/intr/InterruptManager.hh>
#include <kernel/intr/InterruptType.hh>
#include <kernel/mem/MemoryManager.hh>
#include <kernel/pci/Enumerate.hh>
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

usize __stack_chk_guard = 0xdeadc0de;

[[noreturn]] void assertion_failed(const char *file, unsigned int line, const char *expr, const char *msg) {
    kernel::dmesg("\nAssertion '{}' failed at {}:{}", expr, file, line);
    if (msg != nullptr) {
        kernel::dmesg("=> {}", msg);
    }
    while (true) {
        asm volatile("cli; hlt");
    }
}

namespace kernel {

class LocalApic;

namespace {

InterruptPolarity interrupt_polarity(uint8 polarity) {
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

InterruptTriggerMode interrupt_trigger_mode(uint8 trigger_mode) {
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
    auto *madt = xsdt->find<acpi::ApicTable>();
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

    auto *mcfg = xsdt->find<acpi::PciTable>();
    ENSURE(mcfg != nullptr && mcfg->valid());
    pci::enumerate(mcfg);

    auto *fb = new FramebufferDevice(boot_info->framebuffer_base, boot_info->width, boot_info->height,
                                     boot_info->pixels_per_scan_line * sizeof(uint32));
    fb->leak_ref();

    // Mark reclaimable memory as available. Note that this means boot_info is invalid to access after this point.
    MemoryManager::reclaim(boot_info);

    auto init_thread = Thread::create_user(ThreadPriority::Normal);
    EXPECT(init_thread->exec("/bin/system-server"sv));
    Scheduler::insert_thread(ustd::move(init_thread));
    Scheduler::yield_and_kill();
}

} // namespace

[[noreturn]] extern "C" void __stack_chk_fail() {
    auto *rbp = static_cast<uint64 *>(__builtin_frame_address(0));
    while (rbp != nullptr && rbp[1] != 0) {
        dmesg("{:h}", rbp[1]);
        rbp = reinterpret_cast<uint64 *>(*rbp);
    }
    ENSURE_NOT_REACHED("Stack smashing detected!");
}

extern "C" void kmain(BootInfo *boot_info) {
    Console::initialise(boot_info);
    DmesgDevice::early_initialise(boot_info);
    dmesg("core: Using font {} {}", g_font.name(), g_font.style());
    if constexpr (k_kernel_qemu_debug) {
        ENSURE(port_read(0xe9) == 0xe9, "KERNEL_QEMU_DEBUG config option enabled, but port e9 isn't available!");
    }
    if constexpr (k_kernel_stack_protector) {
        dmesg("core: SSP initialised with guard value {:h}", __stack_chk_guard);
    }

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

    auto *madt = xsdt->find<acpi::ApicTable>();
    ENSURE(madt != nullptr && madt->valid());
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

    auto *hpet_table = xsdt->find<acpi::HpetTable>();
    ENSURE(hpet_table != nullptr && hpet_table->valid());
    TimeManager::initialise(hpet_table);

    // Start a new kernel thread that will perform the rest of the initialisation. We do this so that we can safely
    // start kernel threads, and so that the current stack we are using is no longer in use, meaning we can reclaim the
    // memory.
    auto kernel_init_thread = Thread::create_kernel(&kernel_init, ThreadPriority::Normal);
    kernel_init_thread->register_state().rdi = reinterpret_cast<uintptr>(boot_info);
    kernel_init_thread->register_state().rsi = reinterpret_cast<uintptr>(xsdt);

    Scheduler::initialise();
    Scheduler::insert_thread(ustd::move(kernel_init_thread));
    Scheduler::setup();
    Scheduler::start();
}

} // namespace kernel
