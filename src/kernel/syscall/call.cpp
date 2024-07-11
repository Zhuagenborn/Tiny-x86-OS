#include "kernel/syscall/call.h"
#include "kernel/debug/assert.h"
#include "kernel/io/file/dir.h"
#include "kernel/io/file/file.h"
#include "kernel/io/video/console.h"
#include "kernel/io/video/print.h"
#include "kernel/memory/pool.h"
#include "kernel/process/proc.h"

namespace sc {

/**
 * @brief System call handlers.
 *
 * @details
 * When a user application needs to call a kernel method, @p mem::Allocate, for example:
 * 1. They can only call its user version @p usr::mem::Allocate, which is a wrapper for a system call.
 * 2. @p usr::mem::Allocate calls the method @p usr::sc::SysCall with the type @p usr::sc::SysCallType::MemAlloc.
 * 3. @p usr::sc::SysCall generates a @p sys_call_intr_num interrupt in @p src/kernel/syscall/call.asm.
 * 4. The CPU jumps to the interrupt entry point @p SysCallEntry in @p src/kernel/interrupt/intr.asm.
 * 5. @p SysCallEntry calls the corresponding kernel method @p sys_call_handlers[usr::sc::SysCallType::MemAlloc], which is @p mem::Allocate.
 */
extern "C" stl::uintptr_t sys_call_handlers[];

stl::uintptr_t sys_call_handlers[count] {};

SysCallHandlerTab<count>& GetSysCallHandlerTab() noexcept {
    static SysCallHandlerTab<count> handlers {sys_call_handlers};
    return handlers;
}

void InitSysCall() noexcept {
    GetSysCallHandlerTab()
        .Register(SysCallType::GetCurrPid, &tsk::Process::GetCurrPid)
        .Register(SysCallType::PrintChar, &io::Console::PrintChar)
        .Register(SysCallType::PrintHex,
                  static_cast<void (*)(stl::uint32_t)>(&io::Console::PrintHex))
        .Register(SysCallType::PrintStr, static_cast<void (*)(const char*)>(&io::Console::PrintStr))
        .Register(SysCallType::MemAlloc, static_cast<void* (*)(stl::size_t)>(&mem::Allocate))
        .Register(SysCallType::Fork, static_cast<stl::size_t (*)()>(&tsk::Process::ForkCurrent))
        .Register(SysCallType::MemFree, static_cast<void (*)(void*)>(&mem::Free))
        .Register(SysCallType::OpenFile,
                  static_cast<stl::size_t (*)(const io::sc::File::OpenArgs&)>(&io::sc::File::Open))
        .Register(SysCallType::ReadFile,
                  static_cast<stl::size_t (*)(const io::sc::File::ReadArgs&)>(&io::sc::File::Read))
        .Register(SysCallType::SeekFile,
                  static_cast<stl::size_t (*)(const io::sc::File::SeekArgs&)>(&io::sc::File::Seek))
        .Register(
            SysCallType::WriteFile,
            static_cast<stl::size_t (*)(const io::sc::File::WriteArgs&)>(&io::sc::File::Write))
        .Register(SysCallType::CloseFile, static_cast<void (*)(stl::size_t)>(&io::sc::File::Close))
        .Register(SysCallType::DeleteFile,
                  static_cast<bool (*)(const char*)>(&io::sc::File::Delete))
        .Register(SysCallType::CreateDir,
                  static_cast<bool (*)(const char*)>(&io::sc::Directory::Create));

    io::PrintlnStr("System calls have been initialized.");
}

}  // namespace sc