#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/video/print.h"

namespace dbg {

namespace {

/**
 * @brief Show the source code information, display a message and pause the system.
 *
 * @param src The source code information. The developer should not change this parameter.
 * @param msg An optional message to display.
 */
[[noreturn]] void PanicSpin(const stl::source_location& src,
                            const stl::string_view msg = nullptr) noexcept {
    const intr::IntrGuard guard;
    io::PrintlnStr("\n!!!!! System Panic !!!!!");
    io::Printf("\tFile: {}.\n", src.file_name());
    io::Printf("\tLine: 0x{}.\n", src.line());
    io::Printf("\tFunction: {}.\n", src.function_name());
    if (!msg.empty()) {
        io::Printf("\tMessage: {}.\n", msg.data());
    }

    while (true) {
    }
}

}  // namespace

void Assert(bool cond, const stl::string_view msg, const stl::source_location& src) noexcept {
    if constexpr (enabled) {
        if (!cond) {
            PanicSpin(src, msg);
        }
    }
}

}  // namespace dbg