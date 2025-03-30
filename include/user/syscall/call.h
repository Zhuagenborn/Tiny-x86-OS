/**
 * @file call.h
 * @brief User-mode system calls.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::sc {

/**
 * @brief Types of system calls.
 *
 * @warning
 * Their order must be the same as @p sc::SysCallType.
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

extern "C" {

/**
 * @brief Call a kernel method by a system call in user mode.
 *
 * @param func The type of a system call.
 * @param arg A user-defined argument.
 * @return The returned value of the kernel method.
 */
stl::int32_t SysCall(SysCallType func, void* arg = nullptr) noexcept;
}

}  // namespace usr::sc