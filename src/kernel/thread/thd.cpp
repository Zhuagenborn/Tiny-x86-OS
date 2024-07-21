#include "kernel/thread/thd.h"
#include "kernel/io/io.h"
#include "kernel/io/timer.h"
#include "kernel/memory/page.h"
#include "kernel/memory/pool.h"
#include "kernel/process/proc.h"
#include "kernel/process/tss.h"

namespace tsk {

namespace {

struct ThreadLists {
    //! The list for ready threads.
    TagList ready;

    //! The list for all threads.
    TagList all;
};

ThreadLists& GetThreadLists() noexcept {
    static ThreadLists thds;
    return thds;
}

/**
 * @brief The thread startup entry. It calls a callback method with an argument.
 *
 * @details
 * This method will never be called via a @p call instruction like a regular method.
 * When a new thread is scheduled for the first time:
 * 1. @p eip of the new thread's @p SwitchStack is set to the address of this method.
 * 2. When switching to the new thread in the method @p SwitchThread, @p eip is popped and the CPU runs this method.
 *
 * @see @p Start
 */
void StartupCallback(const Thread::Callback callback, void* const arg) noexcept {
    dbg::Assert(callback);
    intr::EnableIntr();
    callback(arg);
}

extern "C" {
//! Switch the running thread.
void SwitchThread(Thread& from, Thread& to) noexcept;

//! Get the current running thread.
Thread& GetCurrThread() noexcept;

//! Halt the CPU until the next external interrupt is fired.
void HaltCpu() noexcept;

//! The label of the interrupt exit, defined in @p src/kernel/interrupt/intr.asm.
[[noreturn]] void intr_exit() noexcept;
}

//! A thread that runs when the system is idle.
void Idle(void*) noexcept {
    while (true) {
        Thread::GetCurrent().Block(Thread::Status::Blocked);
        intr::EnableIntr();
        HaltCpu();
    }
}

/**
 * @brief A wrapper of a global variable representing the idle thread.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
KrnlThread*& GetIdleThread() noexcept {
    static KrnlThread* thd {nullptr};
    return thd;
}

/**
 * @brief A wrapper of a global variable representing the main kernel thread.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
KrnlThread*& GetMainThreadImpl() noexcept {
    static KrnlThread* thd {nullptr};
    return thd;
}

void InitIdleThread() noexcept {
    constexpr stl::size_t priority {10};
    auto& thd {GetIdleThread()};
    dbg::Assert(!thd);
    thd = &KrnlThread::Create("idle", priority, &Idle);
}

/**
 * @brief A wrapper of a global @p bool variable representing whether threads have been initialized.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
bool& IsThreadInitedImpl() noexcept {
    static bool inited {false};
    return inited;
}

}  // namespace

io::FileDesc ProcFileDescTab::SyncGlobal(const io::FileDesc global) noexcept {
    return Thread::GetFileDescTab().SyncGlobal(global);
}

io::FileDesc ProcFileDescTab::GetGlobal(const io::FileDesc local) noexcept {
    return Thread::GetFileDescTab().GetGlobal(local);
}

void ProcFileDescTab::Reset(const io::FileDesc local) noexcept {
    Thread::GetFileDescTab().Reset(local);
}

Thread& Thread::GetByTag(const TagList::Tag& tag, const TagType type) noexcept {
    switch (type) {
        case TagType::AllThreads: {
            return tag.GetElem<Thread, sizeof(TagList::Tag)>();
        }
        default: {
            return tag.GetElem<Thread>();
        }
    }
}

bool Thread::IsKrnlThread() const noexcept {
    return proc_ == nullptr;
}

Thread& Thread::Fork() const noexcept {
    // Create a new thread and copy the current thread data to it.
    const auto child {mem::AllocPages<Thread>(mem::PoolType::Kernel)};
    mem::AssertAlloc(child);
    CopyTo(*child);

    // The child thread should return `0` when exiting the interrupt.
    auto& intr_stack {child->GetIntrStack()};
    intr_stack.eax = reinterpret_cast<stl::uint32_t>(nullptr);

    // When the child thread is scheduled to run,
    // it needs to continue running from the return address of `Fork` as the parent thread.
    // The child thread will start via `SwitchThread`, so we should set its switch stack in advance.
    auto& switch_stack {child->GetSwitchStack()};
    // The return value of `Fork` in the child thread should be `0`,
    // so we have to skip the assignment statement for `EAX`.
    switch_stack.eip = reinterpret_cast<stl::uint32_t>(&intr_exit);
    // `SwitchThread` should restore registers from the switch stack.
    child->krnl_stack_ = static_cast<void*>(&switch_stack);

    dbg::Assert(child->status_ == Status::Died);
    dbg::Assert(!GetThreadLists().all.Find(child->tags_.all_thds));
    GetThreadLists().all.PushBack(child->tags_.all_thds);

    child->status_ = Status::Ready;
    dbg::Assert(!GetThreadLists().ready.Find(child->tags_.general));
    GetThreadLists().ready.PushBack(child->tags_.general);
    return *child;
}

void Thread::CopyTo(Thread& thd) const noexcept {
    stl::memcpy(&thd, this, mem::page_size);
    thd.tags_ = {};
    thd.status_ = Status::Died;
    thd.elapsed_ticks_ = 0;
    thd.krnl_stack_ = reinterpret_cast<void*>(thd.GetKrnlStackBottom() - sizeof(intr::IntrStack)
                                              - sizeof(StartupStack));
    thd.ResetTicks();
}

Thread& Thread::GetByTag(const TagList::Tag& tag) noexcept {
    return GetByTag(tag, TagType::General);
}

Thread& Thread::GetCurrent() noexcept {
    return GetCurrThread();
}

const TagList::Tag& Thread::GetTag() const noexcept {
    return tags_.general;
}

TagList::Tag& Thread::GetTag() noexcept {
    return const_cast<TagList::Tag&>(const_cast<const Thread&>(*this).GetTag());
}

Thread& Thread::Create(const stl::string_view name, const stl::size_t priority,
                       const Callback callback, void* const arg, Process* const proc) noexcept {
    const auto thd {mem::AllocPages<Thread>(mem::PoolType::Kernel)};
    mem::AssertAlloc(thd);
    return thd->Init(name, priority, proc).Start(callback, arg);
}

Thread& Thread::Init(const stl::string_view name, const stl::size_t priority,
                     Process* const proc) noexcept {
    if (!name.empty()) {
        stl::strcpy_s(name_.data(), name_.max_size(), name.data());
    }

    stack_guard_ = stack_guard;
    priority_ = priority;
    remain_ticks_ = priority;
    elapsed_ticks_ = 0;
    krnl_stack_ = reinterpret_cast<void*>(GetKrnlStackBottom() - sizeof(intr::IntrStack)
                                          - sizeof(StartupStack));
    proc_ = proc;
    // The main kernel thread is already running when the system starts.
    status_ = &KrnlThread::GetMain() == this ? Status::Running : Status::Died;

    // The thread has been created, but not ready to run (except the main kernel thread).
    dbg::Assert(!GetThreadLists().all.Find(tags_.all_thds));
    GetThreadLists().all.PushBack(tags_.all_thds);
    return *this;
}

Thread& Thread::Start(const Callback callback, void* const arg) noexcept {
    dbg::Assert(status_ == Status::Died);
    auto& startup_stack {GetStartupStack()};
    startup_stack.eip = reinterpret_cast<stl::uint32_t>(StartupCallback);
    startup_stack.callback = callback;
    startup_stack.arg = arg;
    status_ = Status::Ready;

    // The thread is ready to run. It can be scheduled now.
    dbg::Assert(!GetThreadLists().ready.Find(tags_.general));
    GetThreadLists().ready.PushBack(tags_.general);
    return *this;
}

const intr::IntrStack& Thread::GetIntrStack() const noexcept {
    return *reinterpret_cast<const intr::IntrStack*>(
        reinterpret_cast<const stl::byte*>(GetKrnlStackBottom()) - sizeof(intr::IntrStack));
}

intr::IntrStack& Thread::GetIntrStack() noexcept {
    return const_cast<intr::IntrStack&>(const_cast<const Thread&>(*this).GetIntrStack());
}

const Thread::SwitchStack& Thread::GetSwitchStack() const noexcept {
    return *reinterpret_cast<const SwitchStack*>(reinterpret_cast<const stl::byte*>(&GetIntrStack())
                                                 - sizeof(SwitchStack));
}

Thread::SwitchStack& Thread::GetSwitchStack() noexcept {
    return const_cast<SwitchStack&>(const_cast<const Thread&>(*this).GetSwitchStack());
}

const Thread::StartupStack& Thread::GetStartupStack() const noexcept {
    return *static_cast<const StartupStack*>(krnl_stack_);
}

Thread::StartupStack& Thread::GetStartupStack() noexcept {
    return const_cast<StartupStack&>(const_cast<const Thread&>(*this).GetStartupStack());
}

Thread::Status Thread::GetStatus() const noexcept {
    return status_;
}

stl::size_t Thread::GetPriority() const noexcept {
    return priority_;
}

Thread& Thread::SetStatus(const Status status) noexcept {
    status_ = status;
    return *this;
}

stl::uintptr_t Thread::GetKrnlStackBottom() const noexcept {
    return reinterpret_cast<stl::uintptr_t>(this) + mem::page_size;
}

Process* Thread::GetProcess() const noexcept {
    return proc_;
}

bool Thread::Tick() noexcept {
    ++elapsed_ticks_;
    if (remain_ticks_ != 0) {
        --remain_ticks_;
        return true;
    } else {
        // Time slices run out.
        // The thread should be removed from the CPU.
        return false;
    }
}

FileDescTab<max_open_file_count>& Thread::GetFileDescTab() noexcept {
    auto& thd {GetCurrent()};
    if (thd.IsKrnlThread()) {
        return static_cast<KrnlThread&>(thd).GetFileDescTab();
    } else {
        const auto proc {thd.GetProcess()};
        dbg::Assert(proc);
        return proc->GetFileDescTab();
    }
}

void Thread::Unblock(Thread& thd) noexcept {
    dbg::Assert(thd.status_ == Status::Blocked || thd.status_ == Status::Hanging
                || thd.status_ == Status::Waiting);
    const intr::IntrGuard guard;
    dbg::Assert(!GetThreadLists().ready.Find(thd.GetTag()));
    thd.status_ = Status::Ready;
    // Put the thread at the beginning of the ready list so that it can be scheduled to run soon.
    GetThreadLists().ready.PushFront(thd.GetTag());
}

void Thread::Block(const Status status) noexcept {
    dbg::Assert(status == Status::Blocked || status == Status::Hanging
                || status == Status::Waiting);
    const intr::IntrGuard guard;
    status_ = status;
    Schedule();
}

void Thread::Sleep(stl::size_t milliseconds) noexcept {
    dbg::Assert(io::IsTimerInited());
    milliseconds = stl::max<stl::size_t>(1, milliseconds);

    // Convert milliseconds into ticks.
    constexpr auto milliseconds_per_intr {SecondsToMilliseconds(1) / io::timer_freq_per_second};
    const auto sleep_ticks {RoundUpDivide(milliseconds, milliseconds_per_intr)};
    dbg::Assert(sleep_ticks > 0);
    const auto start_ticks {io::GetTicks()};

    // Keep yielding if sleeping ticks are not enough.
    while (io::GetTicks() - start_ticks < sleep_ticks) {
        Yield();
    }
}

void Thread::Yield() noexcept {
    const intr::IntrGuard guard;
    dbg::Assert(!GetThreadLists().ready.Find(tags_.general));
    status_ = Status::Ready;
    GetThreadLists().ready.PushBack(tags_.general);
    Schedule();
}

Thread& Thread::ResetTicks() noexcept {
    remain_ticks_ = priority_;
    return *this;
}

bool Thread::IsStackValid() const noexcept {
    return stack_guard_ == stack_guard;
}

void Thread::Schedule() noexcept {
    dbg::Assert(!intr::IsIntrEnabled());
    if (GetStatus() == Status::Running) {
        dbg::Assert(!GetThreadLists().ready.Find(tags_.general));
        ResetTicks();
        status_ = Status::Ready;
        GetThreadLists().ready.PushBack(tags_.general);
    }

    // If no thread is ready to run,
    // the idle thread will be unblocked and added to the ready list.
    if (GetThreadLists().ready.IsEmpty()) {
        auto& idle_thd {GetIdleThread()};
        dbg::Assert(idle_thd);
        Thread::Unblock(*idle_thd);
    }

    // Get a thread from the ready list and switch to it.
    dbg::Assert(!GetThreadLists().ready.IsEmpty());
    auto& next {GetByTag(GetThreadLists().ready.Pop())};
    next.LoadKrnlEnv();
    next.status_ = Status::Running;
    SwitchThread(*this, next);
}

Thread& Thread::LoadPageDir() noexcept {
    return const_cast<Thread&>(const_cast<const Thread&>(*this).LoadPageDir());
}

const Thread& Thread::LoadPageDir() const noexcept {
    // `krnl_phy_addr` must be defined as a `static` variable to save the physical address of the kernel page directory table.
    // When `krnl_phy_addr` is evaluated for the first time, `CR3` is referring to the kernel page directory table.
    // So we can get its physical address by the virtual address `mem::page_dir_base`.
    // If it is not `static`, it will be evaluated on each call of `LoadPageDir`.
    // After `CR3` refers to the page directory table of a user process,
    // `krnl_phy_addr` will be the physical address of that process-owned table, instead of the kernel page directory table.
    // In that case, we cannot make `CR3` refer to the kernel page directory table again.
    static const auto krnl_phy_addr {mem::VrAddr {mem::page_dir_base}.GetPhyAddr()};
    const auto proc {GetProcess()};
    io::SetCr3(proc ? mem::VrAddr {proc->GetPageDir()}.GetPhyAddr() : krnl_phy_addr);
    return *this;
}

Thread& Thread::LoadKrnlEnv() noexcept {
    return const_cast<Thread&>(const_cast<const Thread&>(*this).LoadKrnlEnv());
}

const Thread& Thread::LoadKrnlEnv() const noexcept {
    LoadPageDir();
    if (!IsKrnlThread()) {
        // A user thread needs a task state segment to switch privileges.
        GetTaskStateSeg().Update(*this);
    }

    return *this;
}

FileDescTab<max_open_file_count>& KrnlThread::GetFileDescTab() noexcept {
    return const_cast<FileDescTab<max_open_file_count>&>(
        const_cast<const KrnlThread&>(*this).GetFileDescTab());
}

const FileDescTab<max_open_file_count>& KrnlThread::GetFileDescTab() const noexcept {
    return file_descs_;
}

KrnlThread& KrnlThread::Create(const stl::string_view name, const stl::size_t priority,
                               const Callback callback, void* const arg) noexcept {
    const auto thd {mem::AllocPages<KrnlThread>(mem::PoolType::Kernel)};
    mem::AssertAlloc(thd);
    return static_cast<KrnlThread&>(thd->Init(name, priority).Start(callback, arg));
}

KrnlThread& KrnlThread::GetMain() noexcept {
    const auto thd {GetMainThreadImpl()};
    dbg::Assert(thd);
    return *thd;
}

void KrnlThread::InitMain() noexcept {
    auto& thd {GetMainThreadImpl()};
    dbg::Assert(!thd);
    thd = static_cast<KrnlThread*>(&Thread::GetCurrent());
    // We do not need to start the main kernel thread since it is already running.
    thd->Init("main", Process::default_priority);
}

KrnlThread& KrnlThread::Init(const stl::string_view name, const stl::size_t priority) noexcept {
    file_descs_.Init();
    return static_cast<KrnlThread&>(Thread::Init(name, priority, nullptr));
}

bool IsThreadInited() noexcept {
    return IsThreadInitedImpl();
}

void InitThread() noexcept {
    dbg::Assert(!IsThreadInited());
    dbg::Assert(mem::IsMemInited());
    KrnlThread::InitMain();
    InitIdleThread();
    IsThreadInitedImpl() = true;
}

}  // namespace tsk