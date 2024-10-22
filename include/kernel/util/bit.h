/**
 * @file bit.h
 * @brief The bit manipulation.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/cstdint.h"

namespace bit {

//! The number of bits in a byte.
inline constexpr stl::size_t byte_len {8};

//! Get the specified bits in a value.
template <typename T>
constexpr T GetBits(const T val, const stl::size_t begin, const stl::size_t count) noexcept {
    if (count < sizeof(val) * byte_len) {
        const auto mask {((static_cast<T>(1) << count) - 1) << begin};
        return (val & mask) >> begin;
    } else {
        return val;
    }
}

//! Reset the specified bits in a value.
template <typename T>
constexpr void ResetBits(T& val, const stl::size_t begin, const stl::size_t count) noexcept {
    if (count < sizeof(val) * byte_len) {
        const auto mask {((static_cast<T>(1) << count) - 1) << begin};
        val &= ~mask;
    } else {
        val = 0;
    }
}

//! Set the value of the specified bits in a value.
template <typename T, typename Bits>
constexpr void SetBits(T& val, const Bits bits, const stl::size_t begin,
                       const stl::size_t count = sizeof(Bits) * byte_len) noexcept {
    if (count < sizeof(val) * byte_len) {
        ResetBits(val, begin, count);
        const auto mask {((static_cast<T>(1) << count) - 1) << begin};
        val |= (static_cast<T>(bits) << begin) & mask;
    } else {
        val = static_cast<T>(bits);
    }
}

//! Check if a bit is set in a value.
template <typename T>
constexpr bool IsBitSet(const T val, const stl::size_t idx) noexcept {
    return val & (static_cast<T>(1) << idx);
}

//! Set a bit in a value.
template <typename T>
constexpr void SetBit(T& val, const stl::size_t idx) noexcept {
    val |= (static_cast<T>(1) << idx);
}

//! Reset a bit in a value.
template <typename T>
constexpr void ResetBit(T& val, const stl::size_t idx) noexcept {
    val &= ~(static_cast<T>(1) << idx);
}

//! Get a byte from a value.
template <typename T>
constexpr stl::uint8_t GetByte(const T val, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint8_t));
    return static_cast<stl::uint8_t>(GetBits(val, begin, sizeof(stl::uint8_t) * byte_len));
}

//! Get a word from a value.
template <typename T>
constexpr stl::uint16_t GetWord(const T val, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint16_t));
    return static_cast<stl::uint16_t>(GetBits(val, begin, sizeof(stl::uint16_t) * byte_len));
}

//! Get a double word from a value.
template <typename T>
constexpr stl::uint32_t GetDword(const T val, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint32_t));
    return static_cast<stl::uint32_t>(GetBits(val, begin, sizeof(stl::uint32_t) * byte_len));
}

//! Set the value of a byte in a value.
template <typename T>
constexpr void SetByte(T& val, const stl::uint8_t byte, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint8_t));
    SetBits(val, byte, begin);
}

//! Set the value of a word in a value.
template <typename T>
constexpr void SetWord(T& val, const stl::uint16_t word, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint16_t));
    SetBits(val, word, begin);
}

//! Set the value of a double word in a value.
template <typename T>
constexpr void SetDword(T& val, const stl::uint32_t dword, const stl::size_t begin) noexcept {
    static_assert(sizeof(T) >= sizeof(stl::uint32_t));
    SetBits(val, dword, begin);
}

//! Combine low bits and high bits into a new value.
template <typename T, typename Bits>
constexpr T CombineBits(const Bits high, const Bits low) noexcept {
    static_assert(sizeof(T) >= 2 * sizeof(Bits));
    return (static_cast<T>(high) << (sizeof(Bits) * byte_len)) + low;
}

//! Combine a low byte and a high byte into a new word.
constexpr stl::uint16_t CombineBytes(const stl::uint8_t high, const stl::uint8_t low) noexcept {
    return CombineBits<stl::uint16_t>(high, low);
}

//! Combine a low word and a high word into a new double word.
constexpr stl::uint32_t CombineWords(const stl::uint16_t high, const stl::uint16_t low) noexcept {
    return CombineBits<stl::uint32_t>(high, low);
}

//! Combine a low double word and a high double word into a new quad word.
constexpr stl::uint64_t CombineDwords(const stl::uint32_t high, const stl::uint32_t low) noexcept {
    return CombineBits<stl::uint64_t>(high, low);
}

//! Get the low byte from a word.
constexpr stl::uint8_t GetLowByte(const stl::uint16_t val) noexcept {
    return GetByte(val, 0);
}

//! Get the low word from a double word.
constexpr stl::uint16_t GetLowWord(const stl::uint32_t val) noexcept {
    return GetWord(val, 0);
}

//! Get the low double word from a quad word.
constexpr stl::uint32_t GetLowDword(const stl::uint64_t val) noexcept {
    return GetDword(val, 0);
}

//! Get the high byte from a word.
constexpr stl::uint8_t GetHighByte(const stl::uint16_t val) noexcept {
    return GetByte(val, sizeof(stl::uint8_t) * byte_len);
}

//! Get the high word from a double word.
constexpr stl::uint16_t GetHighWord(const stl::uint32_t val) noexcept {
    return GetWord(val, sizeof(stl::uint16_t) * byte_len);
}

//! Get the high double word from a quad word.
constexpr stl::uint32_t GetHighDword(const stl::uint64_t val) noexcept {
    return GetDword(val, sizeof(stl::uint32_t) * byte_len);
}

//! Set the value of the low byte in a word.
constexpr void SetLowByte(stl::uint16_t& val, const stl::uint8_t byte) noexcept {
    SetByte(val, byte, 0);
}

//! Set the value of the low word in a double word.
constexpr void SetLowWord(stl::uint32_t& val, const stl::uint16_t word) noexcept {
    SetWord(val, word, 0);
}

//! Set the value of the low double word in a quad word.
constexpr void SetLowDword(stl::uint64_t& val, const stl::uint32_t dword) noexcept {
    SetDword(val, dword, 0);
}

//! Set the value of the high byte in a word.
constexpr void SetHighByte(stl::uint16_t& val, const stl::uint8_t byte) noexcept {
    SetByte(val, byte, sizeof(stl::uint8_t) * byte_len);
}

//! Set the value of the high word in a double word.
constexpr void SetHighWord(stl::uint32_t& val, const stl::uint16_t word) noexcept {
    SetWord(val, word, sizeof(stl::uint16_t) * byte_len);
}

//! Set the value of the high double word in a quad word.
constexpr void SetHighDword(stl::uint64_t& val, const stl::uint32_t dword) noexcept {
    SetDword(val, dword, sizeof(stl::uint32_t) * byte_len);
}

/**
 * @brief The flag bit checking for enumeration types.
 *
 * @tparam E An enumeration type.
 * @tparam T The underlying type of the enumeration.
 */
template <typename E, typename T = stl::uint32_t>
class Flags {
public:
    constexpr Flags(const T flags = 0) noexcept : flags_ {flags} {}

    constexpr Flags(const E flag) noexcept : flags_ {static_cast<T>(flag)} {}

    constexpr bool IsSet(const E flag) const noexcept {
        return flags_ & static_cast<T>(flag);
    }

    constexpr Flags& Set(const E flag) noexcept {
        flags_ |= static_cast<T>(flag);
        return *this;
    }

    constexpr operator T() const noexcept {
        return flags_;
    }

private:
    T flags_;
};

template <typename E, typename T>
constexpr bool operator==(const Flags<E, T> flags, const E flag) noexcept {
    return flags == static_cast<T>(flag);
}

template <typename E, typename T>
constexpr bool operator&(const Flags<E, T> flags, const E flag) noexcept {
    return flags.IsSet(flag);
}

template <typename E, typename T>
constexpr bool operator!=(const Flags<E, T> flags, const E flag) noexcept {
    return !(flags == flag);
}

}  // namespace bit