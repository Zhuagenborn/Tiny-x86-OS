/**
 * @file proc.h
 * @brief User-mode process management.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::tsk {

//! User-mode process management.
class Process {
public:
    Process() = delete;

    static stl::size_t GetCurrPid() noexcept;

    static stl::size_t Fork() noexcept;
};

}  // namespace usr::tsk