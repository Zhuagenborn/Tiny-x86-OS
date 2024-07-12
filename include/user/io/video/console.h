/**
 * The user-mode thread-safe text console.
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::io {

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