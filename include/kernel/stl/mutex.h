#pragma once

#include "kernel/thread/sync.h"

namespace stl {

class mutex {
public:
    mutex() noexcept = default;

    mutex(const mutex&) = delete;

    void lock() noexcept;

    void unlock() noexcept;

private:
    sync::Mutex mtx_;
};

template <typename Mutex>
class lock_guard {
public:
    explicit lock_guard(Mutex& mtx) noexcept : mtx_ {mtx} {
        mtx.lock();
    }

    lock_guard(const lock_guard&) = delete;

    ~lock_guard() noexcept {
        mtx_.unlock();
    }

private:
    Mutex& mtx_;
};

}  // namespace stl