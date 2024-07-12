/**
 * User-mode memory management.
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::mem {

void* Allocate(stl::size_t size) noexcept;

void Free(void* base) noexcept;

}