#include "kernel/process/tss.h"
#include "kernel/descriptor/gdt/tab.h"
#include "kernel/io/video/print.h"
#include "kernel/selector/sel.h"

namespace tsk {

namespace {
extern "C" {

/**
 * @brief Set the task register.
 *
 * @param sel A selector to a task state segment.
 */
void SetTaskReg(stl::uint16_t sel) noexcept;
}

}  // namespace

TaskStateSeg& GetTaskStateSeg() noexcept {
    static TaskStateSeg tss {.ss0 = sel::krnl_stack, .io_base = sizeof(TaskStateSeg)};
    return tss;
}

TaskStateSeg& TaskStateSeg::Update(const Thread& thd) noexcept {
    esp0 = thd.GetKrnlStackBottom();
    return *this;
}

void InitTaskStateSeg() noexcept {
    dbg::Assert(IsThreadInited());
    auto gdt {gdt::GetGlobalDescTab()};

    // Create a kernel descriptor for the task state segment.
    const auto& tss {GetTaskStateSeg()};
    dbg::Assert(gdt[gdt::idx::tss].IsInvalid());
    gdt[gdt::idx::tss] = {reinterpret_cast<stl::uintptr_t>(&tss),
                          sizeof(tss) - 1,
                          {desc::SysType::Tss32, Privilege::Zero}};

    // Create user descriptors for code and data.
    dbg::Assert(gdt[gdt::idx::usr_code].IsInvalid());
    gdt[gdt::idx::usr_code] = static_cast<desc::SegDesc>(
        desc::Descriptor {gdt[gdt::idx::krnl_code]}.SetDpl(Privilege::Three));

    dbg::Assert(gdt[gdt::idx::usr_data].IsInvalid());
    gdt[gdt::idx::usr_data] = static_cast<desc::SegDesc>(
        desc::Descriptor {gdt[gdt::idx::krnl_data]}.SetDpl(Privilege::Three));

    // Load the task state segment.
    SetTaskReg(sel::tss);
    io::PrintlnStr("The task state segment has been initialized.");
}

}  // namespace tsk