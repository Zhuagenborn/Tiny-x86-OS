#include "kernel/io/video/print.h"
#include "kernel/debug/assert.h"

namespace io {

void PrintlnStr(const stl::string_view str) noexcept {
    PrintStr(str);
    PrintChar('\n');
}

void PrintStr(const stl::string_view str) noexcept {
    if (!str.empty()) {
        PrintStr(str.data());
    }
}

void PrintlnHex(const stl::int32_t num) noexcept {
    PrintHex(num);
    PrintChar('\n');
}

void PrintHex(const stl::int32_t num) noexcept {
    if (num >= 0) {
        PrintHex(static_cast<stl::uint32_t>(num));
    } else {
        PrintChar('-');
        PrintHex(static_cast<stl::uint32_t>(-num));
    }
}

void PrintlnChar(const char ch) noexcept {
    PrintChar(ch);
    PrintChar('\n');
}

void PrintlnHex(const stl::uint32_t num) noexcept {
    PrintHex(num);
    PrintChar('\n');
}

namespace _printf_impl {

void Print(const stl::uint32_t num) noexcept {
    PrintHex(num);
}

void Print(const stl::int32_t num) noexcept {
    PrintHex(num);
}

void Print(const char ch) noexcept {
    PrintChar(ch);
}

void Print(const stl::string_view str) noexcept {
    PrintStr(str);
}

void Print(const char* const str) noexcept {
    dbg::Assert(str);
    PrintStr(str);
}

void Printf(const stl::string_view format) noexcept {
    PrintStr(format);
}

}  // namespace _printf_impl

}  // namespace io