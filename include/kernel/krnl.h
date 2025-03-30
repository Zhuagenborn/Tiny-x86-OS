/**
 * @file krnl.h
 * @brief Basic kernel configurations.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/util/metric.h"

//! The size of the kernel in bytes.
inline constexpr stl::size_t krnl_size {MB(1)};

//! The address of the kernel image when it is loaded.
inline constexpr stl::uintptr_t krnl_base {0xC0000000};

enum class Privilege {
    //! The kernel.
    Zero = 0,

    One = 1,
    Two = 2,

    //! Users.
    Three = 3
};

//! Initialize the kernel.
void InitKernel() noexcept;