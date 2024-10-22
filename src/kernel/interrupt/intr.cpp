#include "kernel/interrupt/intr.h"
#include "kernel/interrupt/pic.h"
#include "kernel/io/io.h"
#include "kernel/io/video/print.h"
#include "kernel/selector/sel.h"

namespace intr {

/**
 * @brief Interrupt handlers.
 *
 * @details
 * When an interrupt @p i occurs:
 * 1. The CPU jumps to the entry point @p intr_entries[i], defined in @p src/interrupt/intr.asm.
 * 2. The interrupt handler @p intr_handlers[i] is called by @p intr_entries[i] after registers are saved.
 */
extern "C" Handler intr_handlers[];

namespace {

extern "C" {

/**
 * @brief The entry points of interrupts, defined in @p src/interrupt/intr.asm.
 *
 * @details
 * They will be registered in the interrupt descriptor table.
 * When an interrupt @p i occurs:
 * 1. The CPU jumps to the entry point @p intr_entries[i].
 * 2. After saving registers, @p intr_entries[i] calls the interrupt handler @p intr_handlers[i].
 */
extern stl::uintptr_t intr_entries[count];

//! Set the interrupt descriptor table register.
void SetIntrDescTabReg(stl::uint16_t limit, stl::uintptr_t base) noexcept;

//! Get the interrupt descriptor table register.
void GetIntrDescTabReg(desc::DescTabReg&) noexcept;
}

void SetIntrDescTabReg(const desc::DescTabReg& reg) noexcept {
    SetIntrDescTabReg(reg.GetLimit(), reg.GetBase());
}

IntrDescTab<count>& GetIntrDescTab() noexcept {
    static IntrDescTab<count> idt;
    return idt;
}

//! Initialize the interrupt descriptor table.
void InitIntrDescTab() noexcept {
    for (stl::size_t i {0}; i != GetIntrDescTab().GetCount(); ++i) {
        GetIntrDescTab()[i] = {
            sel::krnl_code, intr_entries[i], {desc::SysType::Intr32, Privilege::Zero}};
    }

    // The system call will be used by user applications, so its privilege is 3.
    const auto sys_call {static_cast<stl::size_t>(Intr::SysCall)};
    GetIntrDescTab()[sys_call] = {
        sel::krnl_code, intr_entries[sys_call], {desc::SysType::Intr32, Privilege::Three}};
}

void RegisterIntrHandlers() noexcept {
    GetIntrHandlerTab()
        .Register(0x00, "#DE Divide Error")
        .Register(0x01, "#DB Debug Exception")
        .Register(0x02, "NMI Intr")
        .Register(0x03, "#BP Breakpoint Exception")
        .Register(0x04, "#OF Overflow Exception")
        .Register(0x05, "#BR Bound Range Exceeded Exception")
        .Register(0x06, "#UD Invalid Opcode Exception")
        .Register(0x07, "#NM Device Not Available Exception")
        .Register(0x08, "#DF Double Fault Exception")
        .Register(0x09, "Coprocessor Segment Overrun")
        .Register(0x0A, "#TS Invalid TSS Exception")
        .Register(0x0B, "#NP Segment Not Present")
        .Register(0x0C, "#SS Stack Fault Exception")
        .Register(0x0D, "#GP General Protection Exception")
        .Register(Intr::PageFault, "#PF Page-Fault Exception")
        .Register(0x10, "#MF x87 FPU Floating-Point Error")
        .Register(0x11, "#AC Alignment Check Exception")
        .Register(0x12, "#MC Machine-Check Exception")
        .Register(0x13, "#XF SIMD Floating-Point Exception");
}

}  // namespace

Handler intr_handlers[count] {};

desc::DescTabReg GetIntrDescTabReg() noexcept {
    desc::DescTabReg reg;
    GetIntrDescTabReg(reg);
    return reg;
}

IntrHandlerTab<count>& GetIntrHandlerTab() noexcept {
    static IntrHandlerTab<count> handlers {intr_handlers, "Unknown", DefaultIntrHandler};
    return handlers;
}

void InitIntr() noexcept {
    InitIntrDescTab();
    RegisterIntrHandlers();

    const pic::Intr intrs[] {pic::Intr::Keyboard, pic::Intr::Clock, pic::Intr::SlavePic,
                             pic::Intr::PrimaryIdeChnl, pic::Intr::SecondaryIdeChnl};
    pic::InitPgmIntrCtrl({intrs, sizeof(intrs) / sizeof(pic::Intr)});
    SetIntrDescTabReg(GetIntrDescTab().BuildReg());
    io::PrintlnStr("The interrupt descriptor table has been initialized.");
}

extern "C" {

bool IsIntrEnabled() noexcept {
    return io::EFlags::Get().If();
}
}

IntrGuard::IntrGuard() noexcept : enabled_ {intr::IsIntrEnabled()} {
    if (enabled_) {
        intr::DisableIntr();
    }
}

IntrGuard::~IntrGuard() noexcept {
    if (enabled_) {
        intr::EnableIntr();
    }
}

void DefaultIntrHandler(const stl::size_t intr_num) noexcept {
    if (intr_num == 0x27 || intr_num == 0x2F) {
        // Ignore spurious interrupts.
        return;
    }

    io::PrintlnStr("\n!!!!! Exception !!!!!");
    io::Printf("\t0x{} {}\n", intr_num, GetIntrHandlerTab().GetName(intr_num));
    if (static_cast<intr::Intr>(intr_num) == intr::Intr::PageFault) {
        io::Printf("\tThe page fault address is 0x{}.\n", io::GetCr2());
    }

    while (true) {
    }
}

}  // namespace intr