/**
 * @file pic.h
 * @brief *Intel 8259A* Programmable Interrupt Controller.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/span.h"

namespace intr::pic {

//! Interrupt requests.
enum class Intr {
    //! The clock.
    Clock = 0,
    //! The keyboard.
    Keyboard = 1,

    /**
     * @brief The slave *Intel 8259A* chip.
     *
     * @details
     * The IBM extended the computer architecture by adding a second *Intel 8259A* chip,
     * This was possible due to the *Intel 8259A*'s ability to cascade interrupts.
     * When we cascade chips, *Intel 8259A* needs to use one of the interrupt requests to signal the other chip.
     */
    SlavePic = 2,

    //! The primary IDE channel.
    PrimaryIdeChnl = 14,
    //! The secondary IDE channel.
    SecondaryIdeChnl = 15
};

/**
 * @brief Initialize the interrupt controller.
 *
 * @param intrs Interrupts to be enabled.
 */
void InitPgmIntrCtrl(stl::span<Intr> intrs) noexcept;

}  // namespace intr::pic