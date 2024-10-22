/**
 * @file page.h
 * @brief Memory paging.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/krnl.h"
#include "kernel/util/bit.h"
#include "kernel/util/metric.h"

namespace mem {

//! The number of page directory entries in the page directory table.
inline constexpr stl::size_t page_dir_count {1024};
//! The index of the page directory entry (the last one) used to refer to the page directory table itself.
inline constexpr stl::size_t page_dir_self_ref {page_dir_count - 1};
//! The size of a page in bytes.
inline constexpr stl::size_t page_size {KB(4)};

//! Align an address to its page base.
constexpr stl::uintptr_t AlignToPageBase(const stl::uintptr_t addr) noexcept {
    return BackwardAlign(addr, static_cast<stl::uintptr_t>(page_size));
}

//! Calculate the number of pages needed for the memory size in bytes.
constexpr stl::size_t CalcPageCount(const stl::size_t size) noexcept {
    return ForwardAlign(size, page_size) / page_size;
}

/**
 * @brief The page table entry and page directory entry.
 *
 * @details
 * They are a part of memory paging, which allows each process to see a full virtual address space,
 * without actually requiring the full amount of physical memory to be available.
 * Memory paging is based on memory segmentation, it translates the linear address obtained from segmentation into a physical address.
 * In the page directory, each entry points to a page table.
 * In the page table, each entry points to a 4 KB physical page frame.
 *
 * @code
 * -----------------------------------------------------------------------------------------
 *      31-12    11-9   8   7   6   5    4     3     2    1   0
 * ┌────────────┬─────┬───┬───┬───┬───┬─────┬─────┬─────┬───┬───┐
 * │ Base 31-12 │ AVL │ G │ 0 │ D │ A │ PCD │ PWT │ U/S │ W │ P │
 * └────────────┴─────┴───┴───┴───┴───┴─────┴─────┴─────┴───┴───┘
 *                      ▲       ▲   ▲                ▲    ▲   ▲
 *                      │       │   │                │    │   └─ 1: The page presents.
 *                      │       │   │                │    └─ 1: The page is writable.
 *                      │       │   │                └─ 1: The page is at user level.
 *                      │       │   │                   0: The page is at supervisor level.
 *                      │       │   └─ 1: The page has been accessed.
 *                      │       └─ 1: The page is dirty (modified).
 *                      └─ 1: The page is global.
 * @endcode
 *
 * In @p src/boot/loader.asm, the page directory table is initialized as follows:
 *
 * @code
 *     ┌─────────────────────────┐
 *     │           ...           │
 *     ├─────────────────────────┤
 *     │     (1) Page Table      │ ◄─────────┐
 *     ├─────────────────────────┤           │
 * ┌── │     (0) Page Table      │ ◄──┐      │
 * │   ├─────────────────────────┤    │      │
 * │   │ (1023) Directory Entry  │ ───│───┐  │
 * │   ├─────────────────────────┤    │   │  │
 * │   │           ...           │    │   │  │
 * │   ├─────────────────────────┤    │   │  │
 * │   │ (0x301) Directory Entry │ ───│───│──┘
 * │   ├─────────────────────────┤    │   │
 * │   │ (0x300) Directory Entry │ ───┤   │
 * │   ├─────────────────────────┤    │   │
 * │   │           ...           │    │   │
 * │   ├─────────────────────────┤    │   │
 * │   │   (0) Directory Entry   │ ───┘ ◄─┘
 * │   ├─────────────────────────┤
 * └─► │         Kernel          │
 *     └─────────────────────────┘
 * @endcode
 */
class PageEntry {
public:
    constexpr PageEntry(const stl::uint32_t entry = 0) noexcept : entry_ {entry} {}

    /**
     * @brief Create a page entry.
     *
     * @param phy_addr The physical address of a page.
     * @param writable Whether the page is writable.
     * @param supervisor Whether the page i at supervisor level.
     * @param present Whether the page presents.
     */
    constexpr PageEntry(const stl::uintptr_t phy_addr, const bool writable, const bool supervisor,
                        const bool present = true) noexcept :
        PageEntry {Format(phy_addr, writable, supervisor, present)} {}

    constexpr bool IsSupervisor() const noexcept {
        return !bit::IsBitSet(entry_, us_pos);
    }

    constexpr PageEntry& SetSupervisor(const bool supervisor = true) noexcept {
        if (supervisor) {
            bit::ResetBit(entry_, us_pos);
        } else {
            bit::SetBit(entry_, us_pos);
        }

        return *this;
    }

    constexpr bool IsWritable() const noexcept {
        return bit::IsBitSet(entry_, rw_pos);
    }

    constexpr PageEntry& SetWritable(const bool writable = true) noexcept {
        if (writable) {
            bit::SetBit(entry_, rw_pos);
        } else {
            bit::ResetBit(entry_, rw_pos);
        }

        return *this;
    }

    constexpr bool IsPresent() const noexcept {
        return bit::IsBitSet(entry_, p_pos);
    }

    constexpr PageEntry& SetPresent(const bool present = true) noexcept {
        if (present) {
            bit::SetBit(entry_, p_pos);
        } else {
            bit::ResetBit(entry_, p_pos);
        }

        return *this;
    }

    constexpr stl::uintptr_t GetAddress() const noexcept {
        return bit::GetBits(entry_, addr_pos, addr_len) << addr_pos;
    }

    constexpr PageEntry& SetAddress(const stl::uintptr_t phy_addr) noexcept {
        bit::SetBits(entry_, bit::GetBits(phy_addr, addr_pos, addr_len), addr_pos, addr_len);
        return *this;
    }

    constexpr operator stl::uint32_t() const noexcept {
        return entry_;
    }

private:
    static constexpr stl::size_t p_pos {0};
    static constexpr stl::size_t rw_pos {p_pos + 1};
    static constexpr stl::size_t us_pos {rw_pos + 1};
    static constexpr stl::size_t addr_pos {12};
    static constexpr stl::size_t addr_len {20};

    static constexpr stl::uint32_t Format(const stl::uintptr_t phy_addr, const bool writable,
                                          const bool supervisor, const bool present) noexcept {
        return PageEntry {}
            .SetAddress(phy_addr)
            .SetSupervisor(supervisor)
            .SetWritable(writable)
            .SetPresent(present);
    }

    stl::uint32_t entry_;
};

static_assert(sizeof(PageEntry) == sizeof(stl::uint32_t));

/**
 * @brief The virtual address.
 *
 * @details
 * @code
 * -----------------------------------------------
 *  31-22 21-12   11-0
 * │ PDE │ PTE │ Offset |
 *    ▲     ▲      ▲
 *    │     │      └─ The offset in the page.
 *    │     └─ The index of the page table entry.
 *    └─ The index of the page directory entry.
 * -----------------------------------------------
 * @endcode
 */
class VrAddr {
public:
    constexpr VrAddr(const stl::uintptr_t addr = 0) noexcept : addr_ {addr} {}

    constexpr VrAddr(const void* const addr) noexcept :
        VrAddr {reinterpret_cast<stl::uintptr_t>(addr)} {}

    /**
     * @brief Create a virtual address.
     *
     * @param page_dir_entry The index of the page directory entry.
     * @param page_tab_entry The index of the page table entry.
     * @param offset The offset in the page.
     */
    constexpr VrAddr(const stl::size_t page_dir_entry, const stl::size_t page_tab_entry,
                     const stl::uintptr_t offset) noexcept :
        VrAddr {Format(page_dir_entry, page_tab_entry, offset)} {}

    constexpr stl::size_t GetPageDirEntryIdx() const noexcept {
        return bit::GetBits(addr_, page_dir_entry_pos, page_dir_entry_len);
    }

    constexpr VrAddr& SetPageDirEntryIdx(const stl::size_t idx) noexcept {
        bit::SetBits(addr_, idx, page_dir_entry_pos, page_dir_entry_len);
        return *this;
    }

    constexpr stl::size_t GetPageTabEntryIdx() const noexcept {
        return bit::GetBits(addr_, page_tab_entry_pos, page_tab_entry_len);
    }

    constexpr VrAddr& SetPageTabEntryIdx(const stl::size_t idx) noexcept {
        bit::SetBits(addr_, idx, page_tab_entry_pos, page_tab_entry_len);
        return *this;
    }

    constexpr stl::uintptr_t GetOffset() const noexcept {
        return bit::GetBits(addr_, offset_pos, offset_len);
    }

    constexpr VrAddr& SetOffset(const stl::uintptr_t offset) noexcept {
        bit::SetBits(addr_, offset, offset_pos, offset_len);
        return *this;
    }

    constexpr stl::uintptr_t GetPageAddr() const noexcept {
        return addr_ - GetOffset();
    }

    constexpr operator stl::uintptr_t() const noexcept {
        return addr_;
    }

    constexpr bool IsNull() const noexcept {
        return addr_ == 0;
    }

    //! Whether the virtual address is mapped to a physical address.
    constexpr bool IsMapped() const noexcept {
        return GetPageDirEntry().IsPresent() && GetPageTabEntry().IsPresent();
    }

    /**
     * @brief Get the page directory entry.
     *
     * @details
     * @p 0xFFFFF000 can be used to access the page directory table.
     * 1. The high 10 bits of @p 0xFFFFF000 are @p 1023.
     *    The last directory entry indexed @p 1023 points to the directory table itself.
     *    The directory table is treated as a page table because it is accessed from a directory entry.
     * 2. The middle 10 bits of @p 0xFFFFF000 are @p 1023.
     *    The last directory entry indexed @p 1023, which points to the directory table itself, is treated as a page entry.
     * 3. By setting the value of the first 10 bits, we can access different directory entries.
     */
    constexpr PageEntry& GetPageDirEntry() const noexcept {
        const stl::uintptr_t addr {VrAddr {}
                                       .SetPageDirEntryIdx(page_dir_self_ref)
                                       .SetPageTabEntryIdx(page_dir_self_ref)
                                       .SetOffset(GetPageDirEntryIdx() * sizeof(PageEntry))};
        return *reinterpret_cast<PageEntry*>(addr);
    }

    /**
     * @brief Get the page table entry.
     *
     * @details
     * @p 0xFFC00000 can be used to access page tables.
     * 1. The high 10 bits of @p 0xFFC00000 are @p 1023.
     *    The last directory entry indexed @p 1023 points to the directory table itself.
     *    The directory table is treated as a page table because it is accessed from a directory entry.
     * 2. The middle 10 bits of @p 0xFFC00000 are @p 0.
     *    The first directory entry indexed @p 0, which points to the first page table, is treated as a page entry.
     * 3. By setting the value of the first 10 bits, we can access different page tables.
     */
    constexpr PageEntry& GetPageTabEntry() const noexcept {
        const stl::uintptr_t addr {VrAddr {}
                                       .SetPageDirEntryIdx(page_dir_self_ref)
                                       .SetPageTabEntryIdx(GetPageDirEntryIdx())
                                       .SetOffset(GetPageTabEntryIdx() * sizeof(PageEntry))};
        return *reinterpret_cast<PageEntry*>(addr);
    }

    VrAddr& Unmap() noexcept;

    //! Unmap the virtual address.
    const VrAddr& Unmap() const noexcept;

    //! Get the mapped physical address.
    stl::uintptr_t GetPhyAddr() const noexcept;

    //! Map the virtual address to a physical address.
    const VrAddr& MapToPhyAddr(stl::uintptr_t phy_addr) const noexcept;

    VrAddr& MapToPhyAddr(stl::uintptr_t phy_addr) noexcept;

private:
    static constexpr stl::size_t offset_pos {0};
    static constexpr stl::size_t offset_len {12};
    static constexpr stl::size_t page_tab_entry_pos {offset_pos + offset_len};
    static constexpr stl::size_t page_tab_entry_len {10};
    static constexpr stl::size_t page_dir_entry_pos {page_tab_entry_pos + page_tab_entry_len};
    static constexpr stl::size_t page_dir_entry_len {10};

    static constexpr stl::uintptr_t Format(const stl::size_t page_dir_entry,
                                           const stl::size_t page_tab_entry,
                                           const stl::uintptr_t offset) noexcept {
        return VrAddr {}
            .SetPageDirEntryIdx(page_dir_entry)
            .SetPageTabEntryIdx(page_tab_entry)
            .SetOffset(offset);
    }

    stl::uintptr_t addr_;
};

static_assert(sizeof(VrAddr) == sizeof(stl::uintptr_t));

//! The index of the first kernel page directory entry.
inline constexpr stl::size_t krnl_page_dir_start {VrAddr {krnl_base}.GetPageDirEntryIdx()};
//! The number of kernel page directory entries.
inline constexpr stl::size_t krnl_page_dir_count {page_dir_count - krnl_page_dir_start - 1};

//! The physical address of the kernel page directory table.
inline constexpr stl::uintptr_t krnl_page_dir_phy_base {MB(1)};

/**
 * @brief The address of the page directory table.
 *
 * @details
 * Each process has its own page directory table.
 * This address points to different page directory tables according to the value of @p CR3.
 *
 * @note
 * Developers can use @p krnl_page_dir_phy_base to refer to the kernel page directory table.
 */
inline constexpr stl::uintptr_t page_dir_base {0xFFFFF000};

}  // namespace mem