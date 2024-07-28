#include "kernel/io/file/dir.h"
#include "kernel/io/disk/disk.h"

namespace io {

bool Directory::Create(const Path& path) noexcept {
    return GetDefaultPart().CreateDir(path);
}

namespace sc {

bool Directory::Create(const char* const path) noexcept {
    return io::Directory::Create(path);
}

}  // namespace sc

}  // namespace io