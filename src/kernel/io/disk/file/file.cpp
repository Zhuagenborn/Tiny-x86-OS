#include "kernel/io/disk/file/file.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/disk/file/inode.h"

namespace io::fs {

File::File(File&& o) noexcept : flags {o.flags}, inode {o.inode}, pos {o.pos} {
    o.Clear();
}

File& File::Clear() noexcept {
    pos = 0;
    flags = 0;
    inode = nullptr;
    return *this;
}

bool File::IsOpen() const noexcept {
    return inode != nullptr;
}

stl::size_t File::GetNodeIdx() const noexcept {
    return GetNode().idx;
}

void File::Close() noexcept {
    if (IsOpen()) {
        if (flags.IsSet(io::File::OpenMode::WriteOnly)
            || flags.IsSet(io::File::OpenMode::ReadWrite)) {
            dbg::Assert(inode->write_deny);
            inode->write_deny = false;
        }

        inode->Close();
    }

    Clear();
}

IdxNode& File::GetNode() const noexcept {
    dbg::Assert(IsOpen());
    return *inode;
}

FileTab<max_open_file_times>& GetFileTab() noexcept {
    static FileTab<max_open_file_times> files;
    return files;
}

}  // namespace io::fs