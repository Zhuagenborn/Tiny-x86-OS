/**
 * @file pool.h
 * @brief User-mode memory management.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::mem {

//! Allocate virtual memory in bytes in user mode.
void* Allocate(stl::size_t size) noexcept;

//! Free virtual memory in user mode.
void Free(void* base) noexcept;

}