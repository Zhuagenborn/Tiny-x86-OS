#include "kernel/io/video/console.h"
#include "kernel/io/keyboard.h"

namespace io {

stl::mutex& Console::GetMutex() noexcept {
    static stl::mutex mtx;
    return mtx;
}

void Console::Read(char* const buf, const stl::size_t count) noexcept {
    auto& keyboard {GetKeyboardBuffer()};
    for (stl::size_t i {0}; i != count; ++i) {
        buf[i] = keyboard.Pop();
    }
}

void Console::PrintlnStr(const stl::string_view str) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintlnStr(str);
}

void Console::PrintStr(const stl::string_view str) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintStr(str);
}

void Console::PrintlnChar(const char ch) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintlnChar(ch);
}

void Console::PrintlnHex(const stl::uint32_t num) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintlnHex(num);
}

void Console::PrintlnHex(const stl::int32_t num) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintlnHex(num);
}

void Console::PrintChar(const char ch) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintChar(ch);
}

void Console::PrintStr(const char* const str) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintStr(str);
}

void Console::PrintHex(const stl::uint32_t num) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintHex(num);
}

void Console::PrintHex(const stl::int32_t num) noexcept {
    const stl::lock_guard guard {GetMutex()};
    io::PrintHex(num);
}

}  // namespace io