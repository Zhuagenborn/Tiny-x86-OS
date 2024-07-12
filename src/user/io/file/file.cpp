#include "user/io/file/file.h"
#include "user/syscall/call.h"

namespace usr::io {

stl::size_t File::Open(const char* const path, const stl::uint32_t flags) noexcept {
    struct Args {
        const char* path;
        stl::uint32_t flags;
    };

    Args args {path, flags};
    return sc::SysCall(sc::SysCallType::OpenFile, reinterpret_cast<void*>(&args));
}

void File::Close(const stl::size_t desc) noexcept {
    sc::SysCall(sc::SysCallType::CloseFile, reinterpret_cast<void*>(desc));
}

bool File::Delete(const char* const path) noexcept {
    return sc::SysCall(sc::SysCallType::DeleteFile, const_cast<char*>(path));
}

stl::size_t File::Write(const stl::size_t desc, const void* const data,
                        const stl::size_t size) noexcept {
    struct Args {
        stl::size_t desc;
        const void* data;
        stl::size_t size;
    };

    Args args {desc, data, size};
    return sc::SysCall(sc::SysCallType::WriteFile, reinterpret_cast<void*>(&args));
}

stl::size_t File::Seek(const stl::size_t desc, const stl::int32_t offset,
                       const SeekOrigin origin) noexcept {
    struct Args {
        stl::size_t desc;
        stl::int32_t offset;
        SeekOrigin origin;
    };

    Args args {desc, offset, origin};
    return sc::SysCall(sc::SysCallType::SeekFile, reinterpret_cast<void*>(&args));
}

stl::size_t File::Read(const stl::size_t desc, void* const buf, const stl::size_t size) noexcept {
    struct Args {
        stl::size_t desc;
        void* buf;
        stl::size_t size;
    };

    Args args {desc, buf, size};
    return sc::SysCall(sc::SysCallType::ReadFile, reinterpret_cast<void*>(&args));
}

}  // namespace usr::io