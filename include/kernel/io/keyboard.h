/**
 * The *Intel 8042* keyboard controller.
 */

#pragma once

#include "kernel/util/block_queue.h"

namespace io {

//! Initialize the keyboard.
void InitKeyboard() noexcept;

using KeyboardBuffer = BlockQueue<char, 64>;

//! Get the keyboard buffer.
KeyboardBuffer& GetKeyboardBuffer() noexcept;

}  // namespace io