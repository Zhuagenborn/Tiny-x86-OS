/**
 * @file call.h
 * @brief System calls.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/debug/assert.h"

namespace sc {

//! The maximum number of supported system calls.
inline constexpr stl::size_t count {0x60};

//! The system call handler.
using Handler = stl::int32_t (*)(void*) noexcept;

/**
 * @brief Types of system calls.
 *
 * @details
 * Each type corresponds to a kernel function.
 */
enum class SysCallType {
    GetCurrPid,
    PrintChar,
    PrintHex,
    PrintStr,
    MemAlloc,
    MemFree,
    OpenFile,
    CloseFile,
    WriteFile,
    ReadFile,
    SeekFile,
    DeleteFile,
    CreateDir,
    Fork
};

/**
 * @brief The system call handler table.
 *
 * @details
 * It can be regarded as a manager for an array of function pointers.
 */
template <stl::size_t count>
class SysCallHandlerTab {
    static_assert(count > 0);

public:
    constexpr SysCallHandlerTab(stl::uintptr_t (&handlers)[count]) noexcept :
        handlers_ {handlers} {}

    template <typename Handler>
    SysCallHandlerTab& Register(const SysCallType func, const Handler handler) noexcept {
        return Register(static_cast<stl::size_t>(func), reinterpret_cast<stl::uintptr_t>(handler));
    }

    constexpr stl::size_t GetCount() const noexcept {
        return count;
    }

    constexpr const stl::uintptr_t* GetHandlers() const noexcept {
        return handlers_;
    }

private:
    SysCallHandlerTab& Register(const stl::size_t idx, const stl::uintptr_t handler) noexcept {
        dbg::Assert(idx < count);
        handlers_[idx] = handler;
        return *this;
    }

    stl::uintptr_t* handlers_ {nullptr};
};

//! Get the system call handler table.
SysCallHandlerTab<count>& GetSysCallHandlerTab() noexcept;

//! Initialize system calls.
void InitSysCall() noexcept;

}  // namespace sc