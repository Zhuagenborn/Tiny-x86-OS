/**
 * @file proc.h
 * @brief User process management.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/memory/pool.h"
#include "kernel/stl/array.h"
#include "kernel/stl/string_view.h"
#include "kernel/thread/thd.h"

namespace tsk {

//! The user process.
class Process {
public:
    //! The priority of the main thread.
    static constexpr stl::size_t default_priority {31};

    //! Get the current thread's process, or @p nullptr for kernel threads.
    static Process* GetCurrent() noexcept;

    //! Fork the current process.
    static stl::size_t ForkCurrent() noexcept;

    //! Get the current process's ID, or @p 0 for kernel threads.
    static stl::size_t GetCurrPid() noexcept;

    /**
     * @brief Create and start a process.
     *
     * @param name A process name.
     * @param code The code entry.
     */
    static Process& Create(stl::string_view name, void* code) noexcept;

    Process(const Process&) = delete;

    //! Get the virtual address pool of the process.
    const mem::VrAddrPool& GetVrAddrPool() const noexcept;

    mem::VrAddrPool& GetVrAddrPool() noexcept;

    //! Get the memory block descriptor table of the process.
    const mem::MemBlockDescTab& GetMemBlockDescTab() const noexcept;

    mem::MemBlockDescTab& GetMemBlockDescTab() noexcept;

    //! Get the file descriptor file.
    const FileDescTab<max_open_file_count>& GetFileDescTab() const noexcept;

    FileDescTab<max_open_file_count>& GetFileDescTab() noexcept;

    const Thread& GetMainThread() const noexcept;

    Thread& GetMainThread() noexcept;

    //! Get the page directory table of the process.
    const mem::PageEntry* GetPageDir() const noexcept;

    //! Get the parent process's ID, or @p npos if the process has no parent.
    stl::size_t GetPid() const noexcept;

    //! Get the process's ID.
    stl::size_t GetParentPid() const noexcept;

    /**
     * @brief Fork the process.
     *
     * @return
     * Return values are different in two processes.
     * - The child process's ID in the parent process.
     * - @p 0 in the child process.
     */
    stl::size_t Fork() const noexcept;

private:
    //! The virtual base address, same as @em Linux.
    static constexpr stl::uintptr_t image_base {0x8048000};

    //! Allocate a new process ID.
    static stl::size_t CreateNewPid() noexcept;

    Process& Init(stl::string_view name, void* code) noexcept;

    //! Initialize the virtual address pool of the process.
    Process& InitVrAddrPool() noexcept;

    //! Initialize the page directory table of the process.
    Process& InitPageDir() noexcept;

    //! Initialize the memory block descriptor table of the process.
    Process& InitMemBlockDescTab() noexcept;

    Thread& CreateThread(stl::string_view name, stl::size_t priority, Thread::Callback callback,
                         void* arg = nullptr) noexcept;

    //! Copy the file descriptor file to another process.
    const Process& CopyFileDescTabTo(Process&) const noexcept;

    Process& CopyFileDescTabTo(Process&) noexcept;

    //! Copy memory to another process.
    const Process& CopyMemTo(Process&, void* buf, stl::size_t buf_size) const noexcept;

    Process& CopyMemTo(Process&, void* buf, stl::size_t buf_size) noexcept;

    /**
     * @details
     * Each process has its own virtual address space.
     * Different virtual address pools allow processes to use the same virtual address,
     * but mapped to various physical addresses with the help of page directory tables.
     */
    mem::VrAddrPool vr_addrs_;

    mem::MemBlockDescTab mem_block_descs_;

    /**
     * @details
     * Each process has its own page directory table.
     * Different page directory tables map the same virtual address in different processes to various physical addresses.
     */
    mem::PageEntry* page_dir_ {nullptr};

    stl::size_t pid_ {npos};

    stl::size_t parent_pid_ {npos};

    FileDescTab<max_open_file_count> file_descs_;

    Thread* main_thd_ {nullptr};
};

}  // namespace tsk