#include "user/io/video/console.h"
#include "user/syscall/call.h"

namespace usr::io {

void Console::PrintlnStr(const char* const str) noexcept {
    PrintStr(str);
    PrintChar('\n');
}

void Console::PrintlnChar(const char ch) noexcept {
    PrintChar(ch);
    PrintChar('\n');
}

void Console::PrintlnHex(const stl::uint32_t num) noexcept {
    PrintHex(num);
    PrintChar('\n');
}

void Console::PrintlnHex(const stl::int32_t num) noexcept {
    PrintHex(num);
    PrintChar('\n');
}

void Console::PrintChar(const char ch) noexcept {
    sc::SysCall(sc::SysCallType::PrintChar, reinterpret_cast<void*>(ch));
}

void Console::PrintStr(const char* const str) noexcept {
    sc::SysCall(sc::SysCallType::PrintStr, const_cast<char*>(str));
}

void Console::PrintHex(const stl::uint32_t num) noexcept {
    sc::SysCall(sc::SysCallType::PrintHex, reinterpret_cast<void*>(num));
}

void Console::PrintHex(const stl::int32_t num) noexcept {
    if (num >= 0) {
        PrintHex(static_cast<stl::uint32_t>(num));
    } else {
        PrintChar('-');
        PrintHex(static_cast<stl::uint32_t>(-num));
    }
}

}  // namespace usr::io