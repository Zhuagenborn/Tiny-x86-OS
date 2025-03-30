/**
 * @file tss.h
 * @brief The task state segment.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/thread/thd.h"

namespace tsk {

/**
 * @brief The task state segment.
 *
 * @details
 * To support multitasking, CPUs provide two native supports: local descriptor tables and task state segments.
 * They require that each task is equipped with a local descriptor table and a task state segment.
 * The local descriptor table saves data and code.
 * The task state segment saves context state and stack pointers for different privilege levels, etc.
 * Task switching is switching these two structures.
 * But this approach has low performance.
 *
 * @em Linux only creates one task state segment for each CPU and all tasks on each CPU share the same task state segment.
 * When switching tasks, it only needs to update @p ss0 and @p esp0 in the task state segment to the segment address and pointer of the new task's kernel stack.
 * For example, when an interrupt occurs in user mode, the CPU gets the kernel stack from @p ss0 and @p esp0 for interrupt handling.
 * The original task state in user mode are manually saved in the kernel stack using the @p push instruction.
 */
struct TaskStateSeg {
    stl::uint32_t backlink;
    stl::uint32_t esp0;
    stl::uint32_t ss0;
    stl::uint32_t esp1;
    stl::uint32_t ss1;
    stl::uint32_t esp2;
    stl::uint32_t ss2;
    stl::uint32_t cr3;
    stl::uint32_t eip;
    stl::uint32_t eflags;
    stl::uint32_t eax;
    stl::uint32_t ecx;
    stl::uint32_t edx;
    stl::uint32_t ebx;
    stl::uint32_t esp;
    stl::uint32_t ebp;
    stl::uint32_t esi;
    stl::uint32_t edi;
    stl::uint32_t es;
    stl::uint32_t cs;
    stl::uint32_t ss;
    stl::uint32_t ds;
    stl::uint32_t fs;
    stl::uint32_t gs;
    stl::uint32_t ldt;
    stl::uint32_t trace;
    stl::uint32_t io_base;

    //! Update @p esp0 to a thread's kernel stack.
    TaskStateSeg& Update(const Thread&) noexcept;
};

//! Initialize the task state segment.
void InitTaskStateSeg() noexcept;

/**
 * @brief Get the task state segment.
 *
 * @details
 * It is shared by all tasks.
 */
TaskStateSeg& GetTaskStateSeg() noexcept;

}  // namespace tsk