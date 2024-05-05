#include "kernel/stl/semaphore.h"

namespace stl {

binary_semaphore::binary_semaphore(const stl::size_t desired) noexcept {
    sema_.Init(desired);
}

void binary_semaphore::release() noexcept {
    sema_.Increase();
}

void binary_semaphore::acquire() noexcept {
    sema_.Decrease();
}

}  // namespace stl