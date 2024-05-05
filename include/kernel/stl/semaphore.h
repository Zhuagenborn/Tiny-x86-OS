#pragma once

#include "kernel/thread/sync.h"

namespace stl {

class binary_semaphore {
public:
    binary_semaphore(stl::size_t desired = 1) noexcept;

    binary_semaphore(const binary_semaphore&) = delete;

    void release() noexcept;

    void acquire() noexcept;

private:
    sync::Semaphore<1> sema_;
};

}  // namespace stl