#include "user/memory/pool.h"
#include "user/syscall/call.h"

namespace usr::mem {

void* Allocate(const stl::size_t size) noexcept {
    return reinterpret_cast<void*>(
        SysCall(sc::SysCallType::MemAlloc, reinterpret_cast<void*>(size)));
}

void Free(void* const base) noexcept {
    SysCall(sc::SysCallType::MemFree, base);
}

}  // namespace usr::mem