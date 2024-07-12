#include "user/io/file/dir.h"
#include "user/syscall/call.h"

namespace usr::io {

bool Directory::Create(const char* const path) noexcept {
    return sc::SysCall(sc::SysCallType::CreateDir, const_cast<char*>(path));
}

}