/**
 * Thread management.
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/disk/file/file.h"
#include "kernel/io/disk/file/inode.h"
#include "kernel/io/file/file.h"
#include "kernel/io/video/print.h"
#include "kernel/stl/array.h"
#include "kernel/stl/string_view.h"
#include "kernel/util/tag_list.h"

namespace tsk {

class Process;

//! The maximum number of files a process can open.
inline constexpr stl::size_t max_open_file_count {8};

/**
 * Utilities for manipulating the file descriptor table of the current process.
 * They are wrappers for @p FileDescTab.
 */
class ProcFileDescTab {
public:
    ProcFileDescTab() = delete;

    static io::FileDesc SyncGlobal(io::FileDesc global) noexcept;

    static io::FileDesc GetGlobal(io::FileDesc local) noexcept;

    static void Reset(io::FileDesc local) noexcept;
};

/**
 * @brief The file descriptor table.
 *
 * @details
 * Each process and kernel thread has a file descriptor table.
 * It does not save detailed data about open files, only descriptors currently opened by the process or kernel thread.
 * A descriptor is an index to the global file table @p io::fs::FileTab.
 *
 * A process or kernel thread follows the steps below to access a file:
 * 1. Use a local descriptor @p io::FileDesc to get the global descriptor @p io::FileDesc from @p FileDescTab.
 * 2. Use the global descriptor to get the file @p io::fs::File from @p io::fs::FileTab.
 */
template <stl::size_t size>
class FileDescTab {
    static_assert(size > io::std_stream_count);

public:
    FileDescTab() noexcept {
        Init();
    }

    FileDescTab& Init() noexcept {
        dbg::Assert(descs_.size() > io::std_stream_count);
        // The first three descriptors are standard I/O streams.
        descs_[io::std_in] = io::std_in;
        descs_[io::std_out] = io::std_out;
        descs_[io::std_err] = io::std_err;
        for (stl::size_t i {io::std_stream_count}; i != descs_.size(); ++i) {
            descs_[i].Reset();
        }

        return *this;
    }

    constexpr stl::size_t GetSize() const noexcept {
        return descs_.size();
    }

    /**
     * @brief Save a global file descriptor to the process or kernel thread.
     *
     * @param global A global file descriptor.
     * @return A local descriptor for internal file access. It might be invalid if the table is full.
     */
    io::FileDesc SyncGlobal(const io::FileDesc global) noexcept {
        for (stl::size_t i {io::std_stream_count}; i != descs_.size(); ++i) {
            if (!descs_[i].IsValid()) {
                descs_[i] = global;
                return i;
            }
        }

        io::PrintlnStr("The process file table is full.");
        return {};
    }

    /**
     * @brief Get the global file descriptor from the process or kernel thread.
     *
     * @param local A local descriptor.
     * @return The global file descriptor.
     */
    io::FileDesc GetGlobal(const io::FileDesc local) const noexcept {
        dbg::Assert(io::std_stream_count <= local && local < descs_.size());
        const auto global {descs_[local]};
        dbg::Assert(global.IsValid());
        return global;
    }

    FileDescTab& Reset(const io::FileDesc local) noexcept {
        dbg::Assert(io::std_stream_count <= local && local < descs_.size());
        descs_[local].Reset();
        return *this;
    }

    /**
     * @brief Fork the file descriptor table.
     *
     * @details
     * For each open file, forking increases their reference count by one.
     */
    const FileDescTab& Fork() const noexcept {
        auto& file_tab {io::fs::GetFileTab()};
        for (stl::size_t i {io::std_stream_count}; i != descs_.size(); ++i) {
            // If a descriptor refers to an open file, increase its reference count.
            if (const auto desc {descs_[i]}; desc.IsValid()) {
                dbg::Assert(file_tab[desc].IsOpen() && file_tab[desc].GetNode().open_times > 0);
                ++file_tab[desc].GetNode().open_times;
            }
        }

        return *this;
    }

    FileDescTab& Fork() noexcept {
        return const_cast<FileDescTab&>(const_cast<const FileDescTab&>(*this).Fork());
    }

private:
    stl::array<io::FileDesc, size> descs_;
};

#pragma pack(push, 1)

/**
 * @brief The thread.
 *
 * @details
 * Here is the memory layout of a thread block and its stack.
 * The total size is one memory page.
 * ```
 * 0xFFF ┌─────────────────┐ `GetKrnlStackBottom`
 *       │ Interrupt Stack │
 *       ├─────────────────┤ `GetIntrStack`
 *       │┌───────────────┐│
 *       ││ Switch Stack  ││
 *       │└───────────────┘│ `GetSwitchStack`
 *       │  Startup Stack  │
 *       ├─────────────────┤ `GetStartupStack`, `krnl_stack_`
 *       │                 │
 *       │    Free Stack   │
 *       │                 │
 *       ├─────────────────┤
 *       │  Control Block  │
 * 0x000 └─────────────────┘ `this`
 * ```
 *
 * @warning
 * The compiler might allocate more memory for the control block in a thread, especially with debug options.
 * In that case, the size of a thread block is larger than one memory page, and some methods no longer work.
 */
class Thread {
    friend class Process;

public:
    enum class Status { Died, Ready, Running, Blocked, Waiting, Hanging };

    using Callback = void (*)(void*) noexcept;

    static FileDescTab<max_open_file_count>& GetFileDescTab() noexcept;

    static void Unblock(Thread&) noexcept;

    static Thread& GetCurrent() noexcept;

    static Thread& GetByTag(const TagList::Tag&) noexcept;

    /**
     * @brief Create and start a thread.
     *
     * @param name A thread name.
     * @param priority A thread priority.
     * @param callback A callback to be executed by the thread.
     * @param arg An argument to the callback.
     * @param proc The parent process, or @p nullptr for kernel threads.
     */
    static Thread& Create(stl::string_view name, stl::size_t priority, Callback callback,
                          void* arg = nullptr, Process* proc = nullptr) noexcept;

    Thread(const Thread&) = delete;

    Status GetStatus() const noexcept;

    Thread& SetStatus(Status) noexcept;

    stl::size_t GetPriority() const noexcept;

    Process* GetProcess() const noexcept;

    /**
     * Get the bottom address of the kernel thread stack.
     * When switching threads, its value corresponds to @p ESP0 of a task state segment.
     */
    stl::uintptr_t GetKrnlStackBottom() const noexcept;

    //! Reset remaining ticks.
    Thread& ResetTicks() noexcept;

    /**
     * @brief Update the running tick.
     *
     * @return Whether the thread can continue running.
     */
    bool Tick() noexcept;

    /**
     * @brief Block the thread.
     *
     * @param status
     * A new thread status.
     * It can only be @p Status::Blocked, @p Status::Hanging or @p Status::Waiting.
     */
    void Block(Status status) noexcept;

    void Sleep(stl::size_t milliseconds) noexcept;

    /**
     * Temporarily remove the thread from the CPU and schedule another thread to run.
     * The removed thread is marked as ready to run and its remaining tick will not be reset.
     */
    void Yield() noexcept;

    /**
     * Remove the thread from the CPU and schedule another thread to run.
     * If the removed thread is running, its remaining tick will be reset and it is marked as ready to run.
     *
     * @details
     * The scheduler uses the basic First-In-First-Out algorithm.
     * The thread priority only represents the maximum number of time slices a thread can run in the CPU at a time.
     * High-priority threads cannot directly preempt running low-priority threads.
     */
    void Schedule() noexcept;

    /**
     * Whether the stack guard is valid.
     * It should be called before manipulating a thread to ensure that the control block is still valid.
     */
    bool IsStackValid() const noexcept;

    const TagList::Tag& GetTag() const noexcept;

    TagList::Tag& GetTag() noexcept;

    //! Whether the thread is a kernel thread, which does not belong to any user process.
    bool IsKrnlThread() const noexcept;

    //! Fork a new thread.
    Thread& Fork() const noexcept;

protected:
    //! @see @p Tags.
    enum class TagType { General, AllThreads };

    static constexpr stl::size_t name_len {16};
    static constexpr stl::uint32_t stack_guard {0x12345678};

    /**
     * @brief The stack for thread switching.
     *
     * @details
     * When the CPU is about to run a new thread:
     * 1. The kernel saves the registers of the current thread to its stack as this structure.
     * 2. Save the stack address to the current thread block.
     * 3. Get the new stack address from the new thread block.
     * 4. Restore the registers of the new thread from the new thread's stack.
     * 5. Run the new thread from its @p EIP.
     *
     * @see @p SwitchThread
     */
    struct SwitchStack {
        /* These registers are manually pushed by the kernel */
        stl::uint32_t ebp;
        stl::uint32_t ebx;
        stl::uint32_t edi;
        stl::uint32_t esi;

        //! @p eip is automatically pushed by the CPU when @p SwitchThread is called.
        stl::uint32_t eip;
    };

    /**
     * @brief The stack for new thread startup.
     *
     * @details
     * When a new thread is scheduled for the first time, we need it to run its callback.
     * 1. @p eip of the new thread's @p SwitchStack should be set to the address of @p StartupCallback.
     * 2. @p callback and @p arg of the new thread's @p StartupStack should be set to the address of the thread callback and its argument.
     * 3. When switching to the new thread in the method @p SwitchThread, @p eip is popped and the CPU runs @p StartupCallback.
     * 4. @p StartupCallback assumes that it is called by a regular @p call instruction. It gets two arguments @p callback and @p arg from the stack.
     * 5. @p StartupCallback calls @p callback with the argument @p arg.
     *
     * @see @p Start
     */
    struct StartupStack : public SwitchStack {
        /**
         * When @p StartupCallback is called, it assumes that the stack top is a return address.
         * We have to skip this place.
         */
        stl::uint32_t reserved_ret_addr;

        //! A thread callback.
        Callback callback;

        //! An optional callback argument.
        void* arg;
    };

    //! We use two tags to manage a thread in different lists.
    struct Tags {
        /**
         * The tag for other lists, for example:
         * - The list for ready threads.
         * - The list for waiting threads.
         */
        TagList::Tag general;

        //! The tag for the list of all threads.
        TagList::Tag all_thds;
    };

    static Thread& GetByTag(const TagList::Tag&, TagType) noexcept;

    /**
     * Initialize a thread control block and add the thread to the all-thread list.
     * Currently it is not ready to be scheduled.
     */
    Thread& Init(stl::string_view name, stl::size_t priority, Process* proc = nullptr) noexcept;

    /**
     * Set a thread callback and add the thread to the ready list.
     * Now it can be scheduled and run.
     */
    Thread& Start(Callback callback, void* arg = nullptr) noexcept;

    //! Get the interrupt stack used to save the thread environment when an interrupt occurs.
    const intr::IntrStack& GetIntrStack() const noexcept;

    intr::IntrStack& GetIntrStack() noexcept;

    //! Get the switch stack used to save the thread environment when switching threads.
    const SwitchStack& GetSwitchStack() const noexcept;

    SwitchStack& GetSwitchStack() noexcept;

    //! Get the startup stack used to create a new thread.
    const StartupStack& GetStartupStack() const noexcept;

    StartupStack& GetStartupStack() noexcept;

    /**
     * Load the thread environment, including:
     * - The page directory table.
     * - The task state segment.
     */
    Thread& LoadKrnlEnv() noexcept;

    const Thread& LoadKrnlEnv() const noexcept;

    Thread& LoadPageDir() noexcept;

    /**
     * @brief Load the page directory table.
     *
     * @details
     * - If the thread is a kernel thread, it uses the kernel page directory table.
     * - If the thread belongs to a user process, it uses the process's page directory table.
     *   Each user process has its own page directory table.
     */
    const Thread& LoadPageDir() const noexcept;

    //! Copy the thread data to another thread.
    void CopyTo(Thread&) const noexcept;

    Tags tags_;

    //! The address of the free thread stack.
    void* krnl_stack_ {nullptr};

    stl::array<char, name_len + 1> name_;

    Status status_ {Status::Died};

    //! The maximum number of time slices the thread can run in the CPU at a time.
    stl::size_t priority_ {0};

    /**
     * The number of remaining ticks the thread can run in the CPU at this time.
     * When time slices run out, the thread is removed from the CPU and waits to be scheduled again.
     */
    stl::size_t remain_ticks_ {0};

    //! The total number of ticks after thread startup.
    stl::size_t elapsed_ticks_ {0};

    //! The parent process, or @p nullptr for kernel threads.
    Process* proc_ {nullptr};

    //! A guard for stack overflow checking.
    stl::uint32_t stack_guard_ {stack_guard};
};

/**
 * The kernel thread.
 * They are created by the kernel and do not have a parent process.
 */
class KrnlThread : public Thread {
public:
    /**
     * @brief Initialize the main kernel thread.
     *
     * @details
     * When the system starts, the main kernel thread loads and initializes the kernel.
     * After the kernel is loaded, it runs the main event loop.
     * So instead of starting it, the initialization only sets its properties such as name and priority.
     */
    static void InitMain() noexcept;

    static KrnlThread& GetMain() noexcept;

    static KrnlThread& Create(stl::string_view name, stl::size_t priority, Callback callback,
                              void* arg = nullptr) noexcept;

    //! Get the file descriptor file.
    const FileDescTab<max_open_file_count>& GetFileDescTab() const noexcept;

    FileDescTab<max_open_file_count>& GetFileDescTab() noexcept;

private:
    KrnlThread& Init(stl::string_view name, stl::size_t priority) noexcept;

    FileDescTab<max_open_file_count> file_descs_;
};

#pragma pack(pop)

//! Initialize threads.
void InitThread() noexcept;

//! Whether threads have been initialized.
bool IsThreadInited() noexcept;

}  // namespace tsk