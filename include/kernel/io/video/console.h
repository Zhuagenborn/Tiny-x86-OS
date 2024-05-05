/**
 * The thread-safe text console.
 */

#pragma once

#include "kernel/io/video/print.h"
#include "kernel/stl/mutex.h"

namespace io {

class Console {
public:
    Console() = delete;

    static void PrintlnStr(stl::string_view) noexcept;

    static void PrintStr(stl::string_view) noexcept;

    static void PrintStr(const char*) noexcept;

    static void PrintlnChar(char) noexcept;

    static void PrintChar(char) noexcept;

    static void PrintlnHex(stl::uint32_t) noexcept;

    static void PrintHex(stl::uint32_t) noexcept;

    static void PrintlnHex(stl::int32_t) noexcept;

    static void PrintHex(stl::int32_t) noexcept;

    //! Read characters from the keyboard buffer.
    static void Read(char* buf, stl::size_t count) noexcept;

    template <typename... Args>
    static void Printf(const stl::string_view format, const Args... args) noexcept {
        const stl::lock_guard guard {GetMutex()};
        io::Printf(format, args...);
    }

private:
    static stl::mutex& GetMutex() noexcept;
};

}  // namespace io