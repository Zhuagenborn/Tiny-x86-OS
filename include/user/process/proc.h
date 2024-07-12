/**
 * User-mode process management.
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::tsk {

class Process {
public:
    Process() = delete;

    static stl::size_t GetCurrPid() noexcept;

    static stl::size_t Fork() noexcept;
};

}  // namespace usr::tsk