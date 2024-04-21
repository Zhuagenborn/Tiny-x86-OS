/**
 * Segment selectors.
 */

#pragma once

#include "kernel/descriptor/gdt/idx.h"
#include "kernel/krnl.h"
#include "kernel/util/bit.h"

namespace sel {

//! Types of descriptor tables.
enum class DescTabType {
    //! The global descriptor table.
    Gdt = 0,
    //! The local descriptor table.
    Ldt = 1
};

/**
 * @brief
 * The segment selector.
 * It can identify a descriptor in a descriptor table.
 *
 * @details
 * ```
 *    15-3    2   1-0
 * ┌───────┬────┬─────┐
 * │ Index │ TI │ RPL │
 * └───────┴────┴─────┘
 *           ▲
 *           └─ 0: The index is for the global descriptor table.
 *              1: The index is for a local descriptor table.
 * ```
 */
class Selector {
public:
    /**
     * @brief Create a selector.
     *
     * @param tab The type of the descriptor table.
     * @param rpl The requested privilege level.
     * @param idx The descriptor index.
     */
    constexpr Selector(const DescTabType tab, const Privilege rpl, const stl::size_t idx) noexcept {
        if (tab == DescTabType::Ldt) {
            bit::SetBit(sel_, tab_pos);
        }

        bit::SetBits(sel_, static_cast<stl::uint32_t>(rpl), rpl_pos, rpl_len);
        bit::SetBits(sel_, idx, idx_pos, idx_len);
    }

    constexpr Selector(const stl::uint16_t sel = 0) noexcept : sel_ {sel} {}

    constexpr operator stl::uint16_t() const noexcept {
        return sel_;
    }

    constexpr DescTabType GetTabType() const noexcept {
        return bit::IsBitSet(sel_, tab_pos) ? DescTabType::Ldt : DescTabType::Gdt;
    }

    constexpr Privilege GetRpl() const noexcept {
        return static_cast<Privilege>(bit::GetBits(sel_, rpl_pos, rpl_len));
    }

    constexpr Selector& SetRpl(const Privilege rpl) noexcept {
        bit::SetBits(sel_, static_cast<stl::uint32_t>(rpl), rpl_pos, rpl_len);
        return *this;
    }

    constexpr stl::size_t GetIdx() const noexcept {
        return bit::GetBits(sel_, idx_pos, idx_len);
    }

    constexpr Selector& Clear() noexcept {
        sel_ = 0;
        return *this;
    }

private:
    static constexpr stl::size_t rpl_pos {0};
    static constexpr stl::size_t rpl_len {2};
    static constexpr stl::size_t tab_pos {rpl_pos + rpl_len};
    static constexpr stl::size_t idx_pos {tab_pos + 1};
    static constexpr stl::size_t idx_len {13};

    stl::uint16_t sel_ {0};
};

static_assert(sizeof(Selector) == sizeof(stl::uint16_t));

//! The kernel selector for code.
inline constexpr Selector krnl_code {DescTabType::Gdt, Privilege::Zero, gdt::idx::krnl_code};
//! The kernel selector for data.
inline constexpr Selector krnl_data {DescTabType::Gdt, Privilege::Zero, gdt::idx::krnl_data};
//! The kernel selector for the stack.
inline constexpr Selector krnl_stack {krnl_data};
//! The kernel selector for the VGA text buffer.
inline constexpr Selector gs {DescTabType::Gdt, Privilege::Zero, gdt::idx::gs};
//! The kernel selector for the task state segment.
inline constexpr Selector tss {DescTabType::Gdt, Privilege::Zero, gdt::idx::tss};

//! The user selector for code.
inline constexpr Selector usr_code {DescTabType::Gdt, Privilege::Three, gdt::idx::usr_code};
//! The user selector for data.
inline constexpr Selector usr_data {DescTabType::Gdt, Privilege::Three, gdt::idx::usr_data};
//! The user selector for the stack.
inline constexpr Selector usr_stack {usr_data};

}  // namespace sel