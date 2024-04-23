/**
 * Descriptors.
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/krnl.h"
#include "kernel/memory/page.h"
#include "kernel/selector/sel.h"
#include "kernel/stl/array.h"
#include "kernel/util/bit.h"

namespace desc {

//! Types of system descriptors.
enum class SysType {
    //! The 16-bit task state segment.
    Tss16 = 0b0001,
    //! The active 16-bit task state segment.
    BusyTss16 = 0b0011,
    //! The 32-bit task state segment.
    Tss32 = 0b1001,
    //! The active 32-bit task state segment.
    BusyTss32 = 0b1011,
    //! The 16-bit interrupt gate.
    Intr16 = 0b0110,
    //! The 32-bit interrupt gate.
    Intr32 = 0b1110,
    //! The 16-bit call gate.
    Call16 = 0b0100,
    //! The 32-bit call gate.
    Call32 = 0b1100,
    //! The 16-bit trap gate.
    Trap16 = 0b0111,
    //! The 32-bit trap gate.
    Trap32 = 0b1111,
    //! The local descriptor table.
    Ldt = 0b0010,
    //! The task gate.
    Task = 0b0101,
};

//! Types of non-system descriptors.
enum class NonSysType {
    //! Executable code.
    ExecCode = 0b100,
    //! Readable, executable code.
    ReadExecCode = 0b101,
    //! Executable, conforming code.
    ExecConformCode = 0b110,
    //! Readable, executable, conforming code.
    ReadExecConformCode = 0b111,
    //! Readable data.
    ReadData = 0b000,
    //! Readable, writable data.
    ReadWriteData = 0b001,
    //! Readable, expand-down data.
    ReadExtDownData = 0b010,
    //! Readable, writable, expand-down data.
    ReadWriteExtDownData = 0b011
};

/**
 * The descriptor attribute.
 * It is located in the bits `40`-`47` of a descriptor.
 */
class Attribute {
public:
    constexpr Attribute(const stl::uint8_t attr = 0) noexcept : attr_ {attr} {}

    /**
     * @brief Create an attribute for a system descriptor.
     *
     * @param type The system descriptor type.
     * @param dpl The descriptor privilege level.
     * @param present Whether the descriptor is valid.
     */
    constexpr Attribute(const SysType type, const Privilege dpl, const bool present = true) noexcept
        :
        Attribute {true, static_cast<stl::uint8_t>(type), dpl, present} {}

    /**
     * @brief Create an attribute for a non-system descriptor.
     *
     * @param type The non-system descriptor type.
     * @param dpl The descriptor privilege level.
     * @param present Whether the descriptor is valid.
     */
    constexpr Attribute(const NonSysType type, const Privilege dpl,
                        const bool present = true) noexcept :
        Attribute {false, static_cast<stl::uint8_t>(type), dpl, present} {}

    constexpr operator stl::uint8_t() const noexcept {
        return attr_;
    }

    constexpr stl::uint8_t GetType() const noexcept {
        auto type {bit::GetBits(attr_, type_pos, type_len)};
        if (!IsSystem()) {
            bit::ResetBit(type, 0);
        }

        return type;
    }

    constexpr Attribute& SetType(const NonSysType type) noexcept {
        return SetSystem(false).SetType(static_cast<stl::uint8_t>(type));
    }

    constexpr Attribute& SetType(const SysType type) noexcept {
        return SetSystem().SetType(static_cast<stl::uint8_t>(type));
    }

    constexpr Privilege GetDpl() const noexcept {
        return static_cast<Privilege>(bit::GetBits(attr_, dpl_pos, dpl_len));
    }

    constexpr Attribute& SetDpl(const Privilege dpl) noexcept {
        bit::SetBits(attr_, static_cast<stl::uint32_t>(dpl), dpl_pos, dpl_len);
        return *this;
    }

    constexpr Attribute& SetSystem(const bool sys = true) noexcept {
        if (sys) {
            bit::ResetBit(attr_, s_pos);
        } else {
            bit::SetBit(attr_, s_pos);
        }

        return *this;
    }

    constexpr bool IsSystem() const noexcept {
        return !bit::IsBitSet(attr_, s_pos);
    }

    constexpr Attribute& SetPresent(const bool present = true) noexcept {
        if (present) {
            bit::SetBit(attr_, p_pos);
        } else {
            bit::ResetBit(attr_, p_pos);
        }

        return *this;
    }

    constexpr bool IsPresent() const noexcept {
        return bit::IsBitSet(attr_, p_pos);
    }

protected:
    static constexpr stl::size_t type_pos {0};
    static constexpr stl::size_t type_len {4};
    static constexpr stl::size_t s_pos {type_pos + type_len};
    static constexpr stl::size_t dpl_pos {s_pos + 1};
    static constexpr stl::size_t dpl_len {2};
    static constexpr stl::size_t p_pos {dpl_pos + dpl_len};

    static constexpr stl::uint8_t Format(const bool system, const stl::uint8_t type,
                                         const Privilege dpl, const bool present) noexcept {
        return Attribute {}.SetType(type).SetSystem(system).SetDpl(dpl).SetPresent(present);
    }

    constexpr Attribute(const bool sys, const stl::uint8_t type, const Privilege dpl,
                        const bool present) noexcept :
        Attribute {Format(sys, type, dpl, present)} {}

    constexpr Attribute& SetType(const stl::uint8_t type) noexcept {
        bit::SetBits(attr_, type, type_pos, type_len);
        return *this;
    }

    stl::uint8_t attr_;
};

static_assert(sizeof(Attribute) == sizeof(stl::uint8_t));

/**
 * @brief The descriptor.
 *
 * @warning
 * Normally, this class should not be used directly.
 * Developers should use its subclasses to create different types of descriptors.
 */
class Descriptor {
public:
    constexpr Descriptor(const stl::uint64_t desc = 0) noexcept : desc_ {desc} {}

    constexpr operator stl::uint64_t() const noexcept {
        return desc_;
    }

    constexpr bool IsInvalid() const noexcept {
        return desc_ == 0;
    }

    constexpr Attribute GetAttribute() const noexcept {
        return bit::GetByte(desc_, attr_pos);
    }

    constexpr Descriptor& SetAttribute(const Attribute attr) noexcept {
        bit::SetByte(desc_, attr, attr_pos);
        return *this;
    }

    constexpr Privilege GetDpl() const noexcept {
        return GetAttribute().GetDpl();
    }

    constexpr Descriptor& SetDpl(const Privilege dpl) noexcept {
        SetAttribute(GetAttribute().SetDpl(dpl));
        return *this;
    }

    constexpr bool IsSystem() const noexcept {
        return GetAttribute().IsSystem();
    }

    constexpr Descriptor& SetSystem(const bool sys = true) noexcept {
        SetAttribute(GetAttribute().SetSystem(sys));
        return *this;
    }

    constexpr Descriptor& SetPresent(const bool present = true) noexcept {
        SetAttribute(GetAttribute().SetPresent(present));
        return *this;
    }

    constexpr bool IsPresent() const noexcept {
        return GetAttribute().IsPresent();
    }

protected:
    static constexpr stl::size_t attr_pos {40};

    stl::uint64_t desc_;
};

static_assert(sizeof(Descriptor) == sizeof(stl::uint64_t));

/**
 * The gate descriptor.
 * There are four types of gate descriptors:
 * - The task gate descriptor.
 * - The call gate descriptor.
 * - The interrupt gate descriptor.
 * - The trap gate descriptor.
 */
class GateDesc : public Descriptor {
public:
    using Descriptor::Descriptor;

    /**
     * @brief Create a gate descriptor.
     *
     * @param sel
     * The selector for the code segment where the routine is located.
     * Or the selector for a task state segment if a task gate descriptor is created.
     * @param func
     * The entry point of a routine.
     * This parameter can be ignored if a task gate descriptor is created.
     * @param attr The descriptor attribute.
     */
    constexpr GateDesc(const sel::Selector sel, const stl::uintptr_t func,
                       const Attribute attr) noexcept :
        Descriptor {Format(sel, func, attr)} {}

    constexpr stl::uint16_t GetFuncOffsetLow() const noexcept {
        return bit::GetWord(desc_, offset_low_pos);
    }

    constexpr stl::uint16_t GetFuncOffsetHigh() const noexcept {
        return bit::GetWord(desc_, offset_high_pos);
    }

    constexpr stl::uintptr_t GetFuncOffset() const noexcept {
        return bit::CombineWords(GetFuncOffsetHigh(), GetFuncOffsetLow());
    }

    constexpr sel::Selector GetSelector() const noexcept {
        return bit::GetWord(desc_, sel_pos);
    }

    constexpr GateDesc& SetFuncOffset(const stl::uintptr_t func) noexcept {
        bit::SetWord(desc_, bit::GetLowWord(func), offset_low_pos);
        bit::SetWord(desc_, bit::GetHighWord(func), offset_high_pos);
        return *this;
    }

    constexpr GateDesc& SetSelector(const sel::Selector sel) noexcept {
        bit::SetWord(desc_, sel, sel_pos);
        return *this;
    }

private:
    static constexpr stl::size_t offset_low_pos {0};
    static constexpr stl::size_t sel_pos {offset_low_pos + sizeof(stl::uint16_t) * bit::byte_len};
    static_assert(attr_pos
                  == sel_pos + (sizeof(sel::Selector) + sizeof(stl::uint8_t)) * bit::byte_len);
    static constexpr stl::size_t offset_high_pos {attr_pos + sizeof(Attribute) * bit::byte_len};

    static constexpr stl::uint64_t Format(const sel::Selector sel, const stl::uintptr_t func,
                                          const Attribute attr) noexcept {
        return GateDesc {}.SetSelector(sel).SetFuncOffset(func).SetAttribute(attr);
    }
};

static_assert(sizeof(GateDesc) == sizeof(stl::uint64_t));

/**
 * @brief
 * The segment descriptor.
 * It is a part of memory segmentation, used for translating a logical address into a linear address.
 * It describes the memory segment referred to in the logical address.
 *
 * @details
 * ```
 * --------------------------------------------- High 32 bits ---------------------------------------------
 *      31-24    23   22   21   20       19-16     15  14-13  12  11-8       7-0
 * ┌────────────┬───┬─────┬───┬─────┬─────────────┬───┬─────┬───┬──────┬────────────┐
 * │ Base 31-24 │ G │ D/B │ L │ AVL │ Limit 19-16 │ P │ DPL │ S │ TYPE │ Base 23-16 │
 * └────────────┴───┴─────┴───┴─────┴─────────────┴───┴─────┴───┴──────┴────────────┘
 *                ▲    ▲    ▲                       ▲         ▲
 *                │    │    │                       │         └─ 0: The segment is a system segment.
 *                │    │    │                       │            1: The segment is a data or code segment.
 *                │    │    │                       └─ 1: The segment presents.
 *                │    │    └─ 0: The segment is 32-bit.
 *                │    │       1: The segment is 64-bit.
 *                │    └─ D (code segments): 0: The segment is 16-bit.
 *                │                          1: The segment is 32-bit.
 *                │       B (data segments): 0: The offset is 16-bit.
 *                │                          1: The offset is 64-bit.
 *                └─ 0: The limit is in units of bytes.
 *                   1: The limit is in units of 4 KB.
 * --------------------------------------------- Low 32 bits ---------------------------------------------
 *     31-16        15-0
 * ┌───────────┬────────────┐
 * │ Base 15-0 │ Limit 15-0 │
 * └───────────┴────────────┘
 * ```
 */
class SegDesc : public Descriptor {
public:
    using Descriptor::Descriptor;

    /**
     * @brief Create a segment descriptor.
     *
     * @param base The segment address.
     * @param limit The segment limit.
     * @param attr The descriptor attribute.
     * @param large
     * If it is @p true, the limit is in units of 4 KB.
     * Otherwise, the limit is in units of bytes.
     */
    constexpr SegDesc(const stl::uintptr_t base, const stl::size_t limit, const Attribute attr,
                      const bool large = false) noexcept :
        Descriptor {Format(base, limit, attr, large)} {}

    constexpr SegDesc& SetBase(const stl::uintptr_t base) noexcept {
        const auto low {bit::GetBits(base, 0, base_low_len)};
        const auto high {bit::GetByte(base, base_low_len)};
        bit::SetBits(desc_, low, base_low_pos, base_low_len);
        bit::SetByte(desc_, high, base_high_pos);
        return *this;
    }

    constexpr stl::uintptr_t GetBase() const noexcept {
        const auto low {bit::GetBits(desc_, base_low_pos, base_low_len)};
        const auto high {bit::GetByte(desc_, base_high_pos)};
        stl::uintptr_t base {static_cast<stl::uintptr_t>(low)};
        bit::SetByte(base, high, base_low_len);
        return base;
    }

    constexpr SegDesc& SetLimit(const stl::size_t limit) noexcept {
        const auto low {bit::GetWord(limit, 0)};
        const auto high {
            bit::GetBits(limit, sizeof(stl::uint16_t) * bit::byte_len, limit_high_len)};
        bit::SetWord(desc_, low, limit_low_pos);
        bit::SetBits(desc_, high, limit_high_pos, limit_high_len);
        return *this;
    }

    constexpr stl::size_t GetLimit() const noexcept {
        const auto low {bit::GetWord(desc_, limit_low_pos)};
        const auto high {bit::GetBits(desc_, limit_high_pos, limit_high_len)};
        stl::size_t limit {low};
        bit::SetBits(limit, high, sizeof(stl::uint16_t) * bit::byte_len, limit_high_len);
        return limit;
    }

    constexpr stl::size_t GetSize() const noexcept {
        return GetGranularity() * (GetLimit() + 1);
    }

    constexpr SegDesc& SetGranularity(const bool large = true) noexcept {
        if (large) {
            bit::SetBit(desc_, g_pos);
        } else {
            bit::ResetBit(desc_, g_pos);
        }

        return *this;
    }

    constexpr stl::size_t GetGranularity() const noexcept {
        return bit::IsBitSet(desc_, g_pos) ? mem::page_size : 1;
    }

private:
    static constexpr stl::size_t limit_low_pos {0};
    static constexpr stl::size_t base_low_pos {limit_low_pos
                                               + sizeof(stl::uint16_t) * bit::byte_len};
    static constexpr stl::size_t base_low_len {(sizeof(stl::uint16_t) + sizeof(stl::uint8_t))
                                               * bit::byte_len};
    static_assert(attr_pos == base_low_pos + base_low_len);
    static constexpr stl::size_t limit_high_pos {attr_pos + sizeof(Attribute) * bit::byte_len};
    static constexpr stl::size_t limit_high_len {4};
    static constexpr stl::size_t avl_pos {limit_high_pos + limit_high_len};
    static constexpr stl::size_t l_pos {avl_pos + 1};
    static constexpr stl::size_t db_pos {l_pos + 1};
    static constexpr stl::size_t g_pos {db_pos + 1};
    static constexpr stl::size_t base_high_pos {g_pos + 1};

    static constexpr stl::uint64_t Format(const stl::uintptr_t base, const stl::size_t limit,
                                          const Attribute attr, const bool large) noexcept {
        return SegDesc {}.SetBase(base).SetLimit(limit).SetGranularity(large).SetAttribute(attr);
    }
};

static_assert(sizeof(SegDesc) == sizeof(stl::uint64_t));

#pragma pack(push, 1)

/**
 * The descriptor table register.
 * They store the location of a descriptor table.
 */
class DescTabReg {
public:
    constexpr DescTabReg() noexcept = default;

    constexpr DescTabReg(const stl::uintptr_t base, const stl::uint16_t limit) noexcept :
        limit_ {limit}, base_ {base} {}

    constexpr stl::uint16_t GetLimit() const noexcept {
        return limit_;
    }

    constexpr stl::uintptr_t GetBase() const noexcept {
        return base_;
    }

private:
    stl::uint16_t limit_ {0};
    stl::uintptr_t base_ {0};
};

static_assert(sizeof(DescTabReg) == sizeof(stl::uint16_t) + sizeof(stl::uint32_t));

#pragma pack(pop)

/**
 * @brief The descriptor table.
 *
 * @tparam T The descriptor type.
 *
 * @warning
 * This class cannot be used directly.
 * Developers should use its different subclasses depending on the memory layout.
 */
template <typename T>
class DescTab {
    static_assert(sizeof(T) == sizeof(stl::uint64_t));

public:
    virtual stl::size_t GetCount() const noexcept = 0;

    virtual const T* GetData() const noexcept = 0;

    const T& operator[](const stl::size_t idx) const noexcept {
        dbg::Assert(idx < GetCount());
        return GetDesc(idx);
    }

    T& operator[](const stl::size_t idx) noexcept {
        return const_cast<T&>(const_cast<const DescTab&>(*this)[idx]);
    }

    T* GetData() noexcept {
        return const_cast<T*>(const_cast<const DescTab&>(*this).GetData());
    }

    //! Build the corresponding descriptor table register.
    DescTabReg BuildReg() const noexcept {
        return {reinterpret_cast<stl::uintptr_t>(GetData()),
                static_cast<stl::uint16_t>(GetCount() * sizeof(T) - 1)};
    }

protected:
    DescTab() noexcept = default;

    virtual const T& GetDesc(stl::size_t) const noexcept = 0;
};

/**
 * The descriptor table that refers to a contiguous sequence of descriptors.
 */
template <typename T>
class DescTabSpan : public DescTab<T> {
public:
    explicit DescTabSpan(const DescTabReg& reg) noexcept :
        count_ {(reg.GetLimit() + 1) / sizeof(T)}, descs_ {reinterpret_cast<T*>(reg.GetBase())} {}

    stl::size_t GetCount() const noexcept override {
        return count_;
    }

    const T* GetData() const noexcept override {
        return descs_;
    }

protected:
    const T& GetDesc(const stl::size_t idx) const noexcept override {
        return descs_[idx];
    }

    stl::size_t count_;
    T* descs_;
};

/**
 * The descriptor table that uses a built-in array to store descriptors.
 */
template <typename T, stl::size_t count>
class DescTabArray : public DescTab<T> {
public:
    constexpr stl::size_t GetCount() const noexcept override {
        return count;
    }

    constexpr const T* GetData() const noexcept override {
        return descs_.data();
    }

protected:
    const T& GetDesc(const stl::size_t idx) const noexcept override {
        return descs_[idx];
    }

    stl::array<T, count> descs_;
};

}  // namespace desc