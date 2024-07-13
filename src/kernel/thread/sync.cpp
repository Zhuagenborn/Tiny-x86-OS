#include "kernel/thread/sync.h"

namespace sync {

void Mutex::Lock() noexcept {
    auto& curr_thd {tsk::Thread::GetCurrent()};
    // The mutex is held by another thread.
    if (holder_ != &curr_thd) {
        sema_.Decrease();
        holder_ = &curr_thd;
        dbg::Assert(repeat_times_ == 0);
        repeat_times_ = 1;
    } else {
        // Repeatedly lock.
        dbg::Assert(repeat_times_ > 0);
        ++repeat_times_;
    }
}

void Mutex::Unlock() noexcept {
    dbg::Assert(holder_ == &tsk::Thread::GetCurrent());
    if (repeat_times_ == 1) {
        repeat_times_ = 0;
        // Reset the holder before unlocking the mutex.
        holder_ = nullptr;
        sema_.Increase();
    } else {
        // Repeatedly unlock.
        dbg::Assert(repeat_times_ > 1);
        --repeat_times_;
    }
}

}  // namespace sync