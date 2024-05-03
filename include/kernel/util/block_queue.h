/**
 * The block queue.
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/stl/array.h"
#include "kernel/stl/mutex.h"
#include "kernel/thread/thd.h"

/**
 * @brief The block queue based on a circular buffer.
 *
 * @details
 * For a queue of size @p n, the circular buffer has `n + 1` slots.
 * When the queue is empty, `head == tail`.
 * When the queue is full, `head + 1 == tail`.
 *
 * @warning
 * This queue only works on a single-core processor.
 */
template <typename T, stl::size_t n>
class BlockQueue {
    static_assert(n > 0);

public:
    static constexpr stl::size_t GetNextPos(const stl::size_t pos) noexcept {
        return (pos + 1) % (n + 1);
    }

    BlockQueue() noexcept = default;

    BlockQueue(const BlockQueue&) = delete;

    bool IsFull() const noexcept {
        dbg::Assert(!intr::IsIntrEnabled());
        return GetNextPos(head_) == tail_;
    }

    bool IsEmpty() const noexcept {
        dbg::Assert(!intr::IsIntrEnabled());
        return head_ == tail_;
    }

    /**
     * Push an object into the queue.
     * If the queue is full, the current thread will be blocked.
     * When another consumer thread pops elements, the blocked thread will be resumed.
     */
    BlockQueue& Push(T val) noexcept {
        dbg::Assert(!intr::IsIntrEnabled());
        while (IsFull()) {
            // Only one thread can change the waiting producer at a time.
            const stl::lock_guard guard {mtx_};
            // The queue is full. The current thread is waiting as a producer.
            Wait(prod_);
        }

        buf_[head_] = stl::move(val);
        head_ = GetNextPos(head_);
        if (consr_) {
            // Wake up a consumer if it is waiting.
            WakeUp(consr_);
        }

        return *this;
    }

    /**
     * Push an object into the queue.
     * If the queue is empty, the current thread will be blocked.
     * When another producer thread pushes elements, the blocked thread will be resumed.
     */
    T Pop() noexcept {
        dbg::Assert(!intr::IsIntrEnabled());
        while (IsEmpty()) {
            // Only one thread can change the waiting consumer at a time.
            const stl::lock_guard guard {mtx_};
            // The queue is empty. The current thread is waiting as a consumer.
            Wait(consr_);
        }

        const auto val {stl::move(buf_[tail_])};
        tail_ = GetNextPos(tail_);
        if (prod_) {
            // Wake up a producer if it is waiting.
            WakeUp(prod_);
        }

        return val;
    }

private:
    /**
     * @brief Block the current thread.
     *
     * @param[out] waiter
     * The input parameter should be @p nullptr.
     * The output is a pointer to the current thread.
     */
    static void Wait(tsk::Thread*& waiter) noexcept {
        dbg::Assert(!waiter);
        waiter = &tsk::Thread::GetCurrent();
        waiter->Block(tsk::Thread::Status::Blocked);
    }

    /**
     * @brief Wake up a thread.
     *
     * @param waiter
     * A blocked thread.
     * The parameter will be set to @p nullptr after waking it up.
     */
    static void WakeUp(tsk::Thread*& waiter) noexcept {
        dbg::Assert(waiter);
        tsk::Thread::Unblock(*waiter);
        waiter = nullptr;
    }

    mutable stl::mutex mtx_;
    //! A waiting producer thread.
    tsk::Thread* prod_ {nullptr};
    //! A waiting consumer thread.
    tsk::Thread* consr_ {nullptr};
    stl::array<stl::byte, n + 1> buf_;
    stl::size_t head_ {0};
    stl::size_t tail_ {0};
};