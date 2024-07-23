#include "kernel/process/proc.h"
#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/io.h"
#include "kernel/krnl.h"
#include "kernel/memory/page.h"

namespace tsk {

namespace {
inline constexpr stl::uintptr_t usr_stack_base {krnl_base - mem::page_size};

extern "C" {
//! Jump to the exit of interrupt routines.
[[noreturn]] void JmpToIntrExit(const void* intr_stack) noexcept;
}

/**
 * @brief Create and start a user process with a code entry.
 *
 * @details
 * User processes are created in kernel mode as threads.
 * To make them enter user mode, we can only use a interrupt return,
 * which is the only way to lower the privilege.
 * 1. Create a thread with the callback @p StartProcess.
 * 2. In @p StartProcess:
 *     1. Prepare an interrupt stack with user selectors,
 *        and set the return address to the code entry.
 *     2. Jump to the interrupt exit.
 * 3. Registers are restored according to the interrupt stack.
 * 4. The process runs from the code entry.
 */
[[noreturn]] void StartProcess(void* const code) noexcept {
    dbg::Assert(code);
    intr::IntrStack intr_stack {};
    // User processes should use user selectors.
    intr_stack.ds = sel::usr_data;
    intr_stack.es = sel::usr_data;
    intr_stack.fs = sel::usr_data;
    intr_stack.eflags = io::EFlags {}.SetIf();
    intr_stack.old_cs = sel::usr_code;
    intr_stack.old_ss = sel::usr_data;
    // The return address is set to the code entry.
    // When the process returns from the interrupt, it will run this code.
    intr_stack.old_eip = reinterpret_cast<stl::uintptr_t>(code);
    const auto stack {mem::AllocPageAtAddr(mem::PoolType::User, usr_stack_base)};
    mem::AssertAlloc(stack);
    intr_stack.old_esp = reinterpret_cast<stl::uintptr_t>(stack) + mem::page_size;
    JmpToIntrExit(&intr_stack);
}

}  // namespace

Process& Process::Create(const stl::string_view name, void* const code) noexcept {
    const auto proc {mem::AllocPages<Process>(mem::PoolType::Kernel)};
    mem::AssertAlloc(proc);
    return proc->Init(name, code);
}

Process& Process::Init(const stl::string_view name, void* const code) noexcept {
    dbg::Assert(code);
    InitVrAddrPool();
    InitPageDir();
    InitMemBlockDescTab();
    file_descs_.Init();
    pid_ = CreateNewPid();
    parent_pid_ = npos;
    main_thd_ = &CreateThread(name, default_priority, &StartProcess, code);
    return *this;
}

Process* Process::GetCurrent() noexcept {
    return Thread::GetCurrent().GetProcess();
}

stl::size_t Process::ForkCurrent() noexcept {
    const auto proc {GetCurrent()};
    dbg::Assert(proc);
    return proc->Fork();
}

stl::size_t Process::GetCurrPid() noexcept {
    const auto proc {GetCurrent()};
    return proc ? proc->GetPid() : 0;
}

Thread& Process::CreateThread(const stl::string_view name, const stl::size_t priority,
                              const Thread::Callback callback, void* const arg) noexcept {
    return Thread::Create(name, priority, callback, arg, this);
}

Process& Process::InitVrAddrPool() noexcept {
    // This virtual address pool only allocates user-space addresses.
    const auto byte_len {(krnl_base - image_base) / mem::page_size / bit::byte_len};
    const auto page_count {mem::CalcPageCount(byte_len)};
    const auto bits {mem::AllocPages(mem::PoolType::Kernel, page_count)};
    mem::AssertAlloc(bits);
    vr_addrs_.Init(image_base, {bits, byte_len});
    return *this;
}

Process& Process::InitMemBlockDescTab() noexcept {
    mem_block_descs_.Init();
    return *this;
}

Process& Process::InitPageDir() noexcept {
    page_dir_ = mem::AllocPages<mem::PageEntry>(mem::PoolType::Kernel);
    mem::AssertAlloc(page_dir_);

    // All processes copy the same kernel page directory entries to share kernel memory.
    stl::memcpy(
        page_dir_ + mem::krnl_page_dir_start,
        reinterpret_cast<const mem::PageEntry*>(mem::page_dir_base) + mem::krnl_page_dir_start,
        mem::krnl_page_dir_count * sizeof(mem::PageEntry));

    // Make the last page directory entry refer to the page directory table itself.
    const auto phy_addr {mem::VrAddr {page_dir_}.GetPhyAddr()};
    page_dir_[mem::page_dir_self_ref] = {phy_addr, true, false};
    return *this;
}

FileDescTab<max_open_file_count>& Process::GetFileDescTab() noexcept {
    return const_cast<FileDescTab<max_open_file_count>&>(
        const_cast<const Process&>(*this).GetFileDescTab());
}

const FileDescTab<max_open_file_count>& Process::GetFileDescTab() const noexcept {
    return file_descs_;
}

mem::MemBlockDescTab& Process::GetMemBlockDescTab() noexcept {
    return const_cast<mem::MemBlockDescTab&>(
        const_cast<const Process&>(*this).GetMemBlockDescTab());
}

const mem::MemBlockDescTab& Process::GetMemBlockDescTab() const noexcept {
    return mem_block_descs_;
}

mem::VrAddrPool& Process::GetVrAddrPool() noexcept {
    return const_cast<mem::VrAddrPool&>(const_cast<const Process&>(*this).GetVrAddrPool());
}

const mem::PageEntry* Process::GetPageDir() const noexcept {
    return page_dir_;
}

const mem::VrAddrPool& Process::GetVrAddrPool() const noexcept {
    return vr_addrs_;
}

Thread& Process::GetMainThread() noexcept {
    return const_cast<Thread&>(const_cast<const Process&>(*this).GetMainThread());
}

stl::size_t Process::GetPid() const noexcept {
    return pid_;
}

stl::size_t Process::GetParentPid() const noexcept {
    return parent_pid_;
}

const Thread& Process::GetMainThread() const noexcept {
    dbg::Assert(main_thd_);
    return *main_thd_;
}

stl::size_t Process::CreateNewPid() noexcept {
    static stl::size_t pid {0};
    static stl::mutex lock;
    const stl::lock_guard guard {lock};
    return ++pid;
}

const Process& Process::CopyFileDescTabTo(Process& child) const noexcept {
    child.file_descs_ = file_descs_;
    child.file_descs_.Fork();
    return *this;
}

Process& Process::CopyFileDescTabTo(Process& child) noexcept {
    return const_cast<Process&>(const_cast<const Process&>(*this).CopyFileDescTabTo(child));
}

const Process& Process::CopyMemTo(Process& child, void* const buf,
                                  const stl::size_t buf_size) const noexcept {
    dbg::Assert(buf && buf_size >= mem::page_size);
    dbg::Assert(main_thd_ && child.main_thd_);
    dbg::Assert(child.main_thd_->proc_ == &child);
    for (stl::size_t i {0}; i != vr_addrs_.GetBitmap().GetCapacity(); ++i) {
        // If a virtual memory page is allocated, copy it to the child process.
        if (vr_addrs_.GetBitmap().IsAlloc(i)) {
            // Copy data from the current process to a kernel buffer.
            const auto addr {vr_addrs_.GetStartAddr() + i * mem::page_size};
            stl::memcpy(buf, reinterpret_cast<const void*>(addr), mem::page_size);

            // Load the page directory table of the child process to access its virtual memory.
            child.main_thd_->LoadPageDir();
            // Allocate a memory page at the same virtual address in the child process.
            mem::AllocPageAtAddr(mem::PoolType::User, child.vr_addrs_, addr);

            // Copy data from the kernel buffer to the child process.
            stl::memcpy(reinterpret_cast<void*>(addr), buf, mem::page_size);
            main_thd_->LoadPageDir();
        }
    }

    return *this;
}

Process& Process::CopyMemTo(Process& child, void* const buf, const stl::size_t buf_size) noexcept {
    return const_cast<Process&>(const_cast<const Process&>(*this).CopyMemTo(child, buf, buf_size));
}

stl::size_t Process::Fork() const noexcept {
    dbg::Assert(!intr::IsIntrEnabled());
    const auto child {mem::AllocPages<Process>(mem::PoolType::Kernel)};
    mem::AssertAlloc(child);
    child->pid_ = CreateNewPid();
    // The parent of the child process is the current process.
    child->parent_pid_ = pid_;
    child->InitMemBlockDescTab();
    child->InitVrAddrPool();
    child->InitPageDir();
    child->main_thd_ = &main_thd_->Fork();
    child->main_thd_->proc_ = child;
    CopyFileDescTabTo(*child);
    const auto buf {mem::AllocPages(mem::PoolType::Kernel)};
    CopyMemTo(*child, buf, mem::page_size);
    mem::FreePages(buf);
    return child->pid_;
}

}  // namespace tsk