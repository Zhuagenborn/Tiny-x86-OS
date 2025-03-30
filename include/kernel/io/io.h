/**
 * @file io.h
 * @brief Port I/O and register control.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/cstdint.h"
#include "kernel/util/bit.h"

namespace io {

//! The @p EFLAGS register.
class EFlags {
public:
    static EFlags Get() noexcept;

    static void Set(EFlags) noexcept;

    constexpr EFlags(const stl::uint32_t val = 0) noexcept : val_ {val} {
        SetMbs();
    }

    constexpr operator stl::uint32_t() const noexcept {
        return val_;
    }

    constexpr EFlags& Clear() noexcept {
        val_ = 0;
        return SetMbs();
    }

    constexpr bool If() const noexcept {
        return bit::IsBitSet(val_, if_pos);
    }

    constexpr EFlags& SetIf() noexcept {
        bit::SetBit(val_, if_pos);
        return *this;
    }

    constexpr EFlags& ResetIf() noexcept {
        bit::ResetBit(val_, if_pos);
        return *this;
    }

private:
    static constexpr stl::size_t if_pos {9};

    constexpr EFlags& SetMbs() noexcept {
        bit::SetBit(val_, 1);
        return *this;
    }

    stl::uint32_t val_;
};

static_assert(sizeof(EFlags) == sizeof(stl::uint32_t));

extern "C" {

//! Get the value of @p CR2.
stl::uint32_t GetCr2() noexcept;

//! Set the value of @p CR3.
void SetCr3(stl::uint32_t) noexcept;

//! Write a byte to a port.
void WriteByteToPort(stl::uint16_t port, stl::byte data) noexcept;

//! Write a number of words to a port.
void WriteWordsToPort(stl::uint16_t port, const void* data, stl::size_t count = 1) noexcept;

//! Read a byte from a port.
stl::byte ReadByteFromPort(stl::uint16_t port) noexcept;

//! Read a number of words from a port.
void ReadWordsFromPort(stl::uint16_t port, void* buf, stl::size_t count = 1) noexcept;
}

}  // namespace io