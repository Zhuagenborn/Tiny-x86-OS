#include "kernel/util/format.h"

stl::size_t ConvertUIntToString(char* const buf, const stl::uint32_t num,
                                const stl::size_t base) noexcept {
    dbg::Assert(buf);
    const auto digit {num % base};
    const auto remain {num / base};
    stl::size_t len {0};
    if (remain > 0) {
        len = ConvertUIntToString(buf, remain, base);
    }

    buf[len] = digit < 10 ? digit + '0' : digit - 10 + 'A';
    buf[len + 1] = '\0';
    return len + 1;
}

stl::size_t ConvertIntToString(char* const buf, const stl::int32_t num,
                               const stl::size_t base) noexcept {
    dbg::Assert(buf);
    if (num < 0) {
        buf[0] = '-';
        return ConvertUIntToString(buf + 1, -num, base) + 1;
    } else {
        return ConvertUIntToString(buf, num, base);
    }
}

namespace _format_string_buffer_impl {

stl::size_t Format(char* const buf, const stl::uint32_t num) noexcept {
    dbg::Assert(buf);
    return ConvertUIntToString(buf, num, 10);
}

stl::size_t Format(char* const buf, const stl::int32_t num) noexcept {
    dbg::Assert(buf);
    return ConvertIntToString(buf, num, 10);
}

stl::size_t Format(char* const buf, const char ch) noexcept {
    dbg::Assert(buf);
    buf[0] = ch;
    return 1;
}

stl::size_t Format(char* const buf, const char* const str) noexcept {
    dbg::Assert(buf && str);
    stl::strcpy(buf, str);
    return stl::strlen(str);
}

stl::size_t Format(char* const buf, const stl::string_view str) noexcept {
    dbg::Assert(buf && !str.empty());
    stl::strcpy(buf, str.data());
    return str.size();
}

stl::size_t FormatStringBuffer(char* const buf, const stl::string_view format) noexcept {
    dbg::Assert(buf);
    return !format.empty() ? Format(buf, format) : 0;
}

}  // namespace _format_string_buffer_impl