/**
 * @file console.h
 * @brief The user-mode thread-safe text console.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::io {

//! The user-mode thread-safe text console.
class Console {
public:
    Console() = delete;

    static void PrintlnStr(const char*) noexcept;

    static void PrintStr(const char*) noexcept;

    static void PrintlnChar(char) noexcept;

    static void PrintChar(char) noexcept;

    static void PrintlnHex(stl::uint32_t) noexcept;

    static void PrintHex(stl::uint32_t) noexcept;

    static void PrintlnHex(stl::int32_t) noexcept;

    static void PrintHex(stl::int32_t) noexcept;
};

}  // namespace usr::io