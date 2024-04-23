/**
 * The global descriptor table.
 */

#pragma once

#include "kernel/descriptor/desc.h"

namespace gdt {

/**
 * @brief The global descriptor table.
 *
 * @note
 * The content of the global descriptor table is defined in @p src/boot/loader.asm,
 * so we should use a span to refer to it.
 */
using GlobalDescTab = desc::DescTabSpan<desc::SegDesc>;

//! Get the global descriptor table register.
desc::DescTabReg GetGlobalDescTabReg() noexcept;

//! Get the global descriptor table.
GlobalDescTab GetGlobalDescTab() noexcept;

}  // namespace gdt