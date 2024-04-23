#include "kernel/descriptor/gdt/tab.h"

namespace gdt {

namespace {

extern "C" {
//! Set the global descriptor table register.
void SetGlobalDescTabReg(stl::uint16_t limit, stl::uintptr_t base) noexcept;

//! Get the global descriptor table register.
void GetGlobalDescTabReg(desc::DescTabReg&) noexcept;
}

void SetGlobalDescTabReg(const desc::DescTabReg& reg) noexcept {
    SetGlobalDescTabReg(reg.GetLimit(), reg.GetBase());
}

}  // namespace

desc::DescTabReg GetGlobalDescTabReg() noexcept {
    desc::DescTabReg reg;
    GetGlobalDescTabReg(reg);
    return reg;
}

GlobalDescTab GetGlobalDescTab() noexcept {
    return GlobalDescTab {GetGlobalDescTabReg()};
}

}  // namespace gdt