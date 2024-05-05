#include "kernel/stl/mutex.h"

namespace stl {

void mutex::lock() noexcept {
    mtx_.Lock();
}

void mutex::unlock() noexcept {
    mtx_.Unlock();
}

}  // namespace stl