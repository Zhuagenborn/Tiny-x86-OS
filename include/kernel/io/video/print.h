/**
 * @file print.h
 * @brief Text printing.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/stl/cstdint.h"
#include "kernel/stl/string_view.h"

namespace io {

//! The screen width in VGA text mode.
inline constexpr stl::size_t text_screen_width {80};
//! The screen height in VGA text mode.
inline constexpr stl::size_t text_screen_height {25};

//! Print a string with a new line.
void PrintlnStr(stl::string_view) noexcept;

//! Print a string.
void PrintStr(stl::string_view) noexcept;

//! Print a character with a new line.
void PrintlnChar(char) noexcept;

//! Print an unsigned hexadecimal integer with a new line.
void PrintlnHex(stl::uint32_t) noexcept;

//! Print a hexadecimal integer with a new line.
void PrintlnHex(stl::int32_t) noexcept;

//! Print a hexadecimal integer.
void PrintHex(stl::int32_t) noexcept;

extern "C" {

//! Print a character.
void PrintChar(char) noexcept;

//! Print a string.
void PrintStr(const char*) noexcept;

//! Print an unsigned hexadecimal integer.
void PrintHex(stl::uint32_t) noexcept;

//! Set the cursor position.
void SetCursorPos(stl::uint16_t) noexcept;

//! Get the cursor position.
stl::uint16_t GetCursorPos() noexcept;
}

namespace _printf_impl {

void Print(stl::uint32_t) noexcept;

void Print(stl::int32_t) noexcept;

void Print(char) noexcept;

void Print(stl::string_view) noexcept;

void Print(const char*) noexcept;

void Printf(stl::string_view) noexcept;

template <typename Arg, typename... Args>
void Printf(const stl::string_view format, const Arg arg, const Args... args) noexcept {
    for (stl::size_t i {0}; i != format.size(); ++i) {
        if (format[i] == '{' && i + 1 != format.size() && format[i + 1] == '}') {
            Print(arg);
            return Printf(format.substr(i + 2), args...);
        } else {
            Print(format[i]);
        }
    }
}

}  // namespace _printf_impl

/**
 * @brief Print variadic values.
 *
 * @param format
 * A format string with a number of @p {}.
 * They will be replaced by the string representations of the arguments.
 * The following types are supported:
 * - `const char*`
 * - `char`
 * - `stl::string_view`
 * - `stl::uint32_t`
 * - `stl::int32_t`
 * @param args Variadic arguments to be printed.
 */
template <typename... Args>
void Printf(const stl::string_view format, const Args... args) noexcept {
    dbg::Assert(!format.empty());
    _printf_impl::Printf(format, args...);
}

}  // namespace io