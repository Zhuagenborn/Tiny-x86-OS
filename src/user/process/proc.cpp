#include "user/process/proc.h"
#include "user/syscall/call.h"

namespace usr::tsk {
stl::size_t Process::GetCurrPid() noexcept {
    return SysCall(sc::SysCallType::GetCurrPid);
}

stl::size_t Process::Fork() noexcept {
    return SysCall(sc::SysCallType::Fork);
}

}  // namespace usr::tsk