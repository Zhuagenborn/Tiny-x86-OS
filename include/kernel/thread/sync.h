/**
 * @file sync.h
 * @brief Multi-threading synchronization.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/thread/thd.h"
#include "kernel/util/tag_list.h"

namespace sync {

template <stl::size_t max>
class Semaphore {
public:
    Semaphore() noexcept = default;

    Semaphore(const stl::size_t val) noexcept {
        Init(val);
    }

    Semaphore(const Semaphore&) = delete;

    Semaphore& Init(const stl::size_t val) noexcept {
        dbg::Assert(val <= max);
        val_ = val;
        return *this;
    }

    void Increase() noexcept {
        const intr::IntrGuard guard;
        dbg::Assert(val_ <= max);
        if (val_ != max) {
            if (!waiters_.IsEmpty()) {
                // Wakes up a waiting thread.
                auto& next_thd {tsk::Thread::GetByTag(waiters_.Pop())};
                tsk::Thread::Unblock(next_thd);
            }

            ++val_;
        }
    }

    void Decrease() noexcept {
        const intr::IntrGuard guard;
        auto& curr_thd {tsk::Thread::GetCurrent()};
        // Keep waiting until the semaphore is not zero.
        while (val_ == 0) {
            // Push the current thread into the wait queue and then block it.
            dbg::Assert(!waiters_.Find(curr_thd.GetTag()));
            waiters_.PushBack(curr_thd.GetTag());
            curr_thd.Block(tsk::Thread::Status::Blocked);
            // When the thread is woken up by the method `Increase`,
            // it is possible that another thread may have grabbed the semaphore faster than it.
            // So we use a loop to check the semaphore again.
        }

        --val_;
    }

private:
    stl::size_t val_ {max};

    //! The threads waiting for a semaphore.
    TagList waiters_;
};

class Mutex {
public:
    Mutex() noexcept = default;

    Mutex(const Mutex&) = delete;

    void Lock() noexcept;

    void Unlock() noexcept;

private:
    //! A mutex is a semaphore with a maximum value of 1.
    Semaphore<1> sema_;

    //! The thread currently holding the mutex.
    tsk::Thread* holder_ {nullptr};

    //! The number of times it is locked.
    stl::size_t repeat_times_ {0};
};

}  // namespace sync