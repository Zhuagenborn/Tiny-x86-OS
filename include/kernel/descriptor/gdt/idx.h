/**
 * @file idx.h
 * @brief Global descriptor indexes.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/cstdint.h"

namespace gdt {

//! The maximum number of global descriptors.
inline constexpr stl::size_t count {60};

namespace idx {

//! The kernel descriptor for code.
inline constexpr stl::size_t krnl_code {1};
//! The kernel descriptor for data.
inline constexpr stl::size_t krnl_data {2};
//! The kernel descriptor for the VGA text buffer.
inline constexpr stl::size_t gs {3};
//! The kernel descriptor for the task state segment.
inline constexpr stl::size_t tss {4};

//! The user descriptor for code.
inline constexpr stl::size_t usr_code {5};
//! The user descriptor for data.
inline constexpr stl::size_t usr_data {6};

}  // namespace idx

}  // namespace gdt