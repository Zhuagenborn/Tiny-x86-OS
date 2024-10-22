/**
 * @file assert.h
 * @brief  * Diagnostics tools.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/source_location.h"
#include "kernel/stl/string_view.h"

namespace dbg {

#ifdef NDEBUG
inline constexpr bool enabled {false};
#else
inline constexpr bool enabled {true};
#endif

/**
 * @brief
 * Check for a condition.
 * If it is @p false, the method displays a message, shows the source code information and pauses the system.
 *
 * @param cond A condition to evaluate.
 * @param msg An optional message to display.
 * @param src The source code information. The developer should not change this parameter.
 */
void Assert(bool cond, stl::string_view msg = nullptr,
            const stl::source_location& src = stl::source_location::current()) noexcept;

}  // namespace dbg