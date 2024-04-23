/**
 * String formatting.
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/stl/string_view.h"

//! Convert an unsigned integer to a string and write it to a buffer.
stl::size_t ConvertUIntToString(char* buf, stl::uint32_t num, stl::size_t base = 10) noexcept;

//! Convert an integer to a string and write it to a buffer.
stl::size_t ConvertIntToString(char* buf, stl::int32_t num, stl::size_t base = 10) noexcept;

namespace _format_string_buffer_impl {

stl::size_t Format(char* buf, stl::uint32_t) noexcept;

stl::size_t Format(char* buf, stl::int32_t) noexcept;

stl::size_t Format(char* buf, char) noexcept;

stl::size_t Format(char* buf, const char*) noexcept;

stl::size_t Format(char* buf, stl::string_view) noexcept;

stl::size_t FormatStringBuffer(char* buf, stl::string_view) noexcept;

template <typename Arg, typename... Args>
stl::size_t FormatStringBuffer(char* const buf, const stl::string_view format, const Arg arg,
                               const Args... args) noexcept {
    dbg::Assert(buf);
    stl::size_t len {0};
    for (stl::size_t i {0}; i != format.size(); ++i) {
        if (format[i] == '{' && i + 1 != format.size() && format[i + 1] == '}') {
            len += Format(buf + len, arg);
            return len + FormatStringBuffer(buf + len, format.substr(i + 2), args...);
        } else {
            buf[len++] = format[i];
        }
    }

    return len;
}

}  // namespace _format_string_buffer_impl

/**
 * @brief Convert variadic values to a string and write it to a buffer.
 *
 * @param buf A string buffer.
 * @param format
 * A format string with a number of @p {}.
 * They will be replaced by the string representations of the arguments.
 * The following types are supported:
 * - `const char*`
 * - `char`
 * - `stl::string_view`
 * - `stl::uint32_t`
 * - `stl::int32_t`
 * @param args Variadic arguments to be converted.
 */
template <typename... Args>
stl::size_t FormatStringBuffer(char* const buf, const stl::string_view format,
                               const Args... args) noexcept {
    dbg::Assert(buf && !format.empty());
    return _format_string_buffer_impl::FormatStringBuffer(buf, format, args...);
}