#include "kernel/memory/page.h"
#include "kernel/debug/assert.h"
#include "kernel/memory/pool.h"
#include "kernel/stl/cstring.h"

namespace mem {

namespace {

extern "C" {
//! Invalidate a Translation Lookaside Buffer (TLB) entry.
void DisableTlbEntry(stl::uintptr_t vr_addr) noexcept;
}

}  // namespace

const VrAddr& VrAddr::MapToPhyAddr(const stl::uintptr_t phy_addr) const noexcept {
    auto& page_dir_entry {GetPageDirEntry()};
    auto& page_tab_entry {GetPageTabEntry()};
    if (!page_dir_entry.IsPresent()) {
        // Allocate a new page for the page table.
        const auto page_tab_phy_base {GetPhyMemPagePool(PoolType::Kernel).AllocPages()};
        AssertAlloc(page_tab_phy_base);
        // Make the page directory entry point to the new page table.
        page_dir_entry.SetAddress(page_tab_phy_base)
            .SetSupervisor(false)
            .SetWritable()
            .SetPresent();
        // Clear the new page table.
        const auto page_tab_base {VrAddr {&page_tab_entry}.GetPageAddr()};
        stl::memset(reinterpret_cast<void*>(page_tab_base), 0, page_size);
    }

    dbg::Assert(!page_tab_entry.IsPresent());
    page_tab_entry.SetAddress(phy_addr).SetSupervisor(false).SetWritable().SetPresent();
    return *this;
}

VrAddr& VrAddr::MapToPhyAddr(const stl::uintptr_t phy_addr) noexcept {
    return const_cast<VrAddr&>(const_cast<const VrAddr&>(*this).MapToPhyAddr(phy_addr));
}

VrAddr& VrAddr::Unmap() noexcept {
    return const_cast<VrAddr&>(const_cast<const VrAddr&>(*this).Unmap());
}

const VrAddr& VrAddr::Unmap() const noexcept {
    if (GetPageDirEntry().IsPresent()) {
        GetPageTabEntry().SetPresent(false);
        DisableTlbEntry(addr_);
    }

    return *this;
}

stl::uintptr_t VrAddr::GetPhyAddr() const noexcept {
    return GetPageTabEntry().GetAddress() + GetOffset();
}

}  // namespace mem