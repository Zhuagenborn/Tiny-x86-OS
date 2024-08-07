#include "kernel/io/disk/file/dir.h"
#include "kernel/io/disk/disk.h"
#include "kernel/io/disk/file/inode.h"
#include "kernel/memory/pool.h"

namespace io::fs {

void Directory::Close() noexcept {
    if (this != &GetRootDir()) {
        dbg::Assert(IsOpen());
        inode->Close();
        mem::Free(this);
    }
}

void Directory::Rewind() noexcept {
    pos = 0;
}

bool Directory::IsEmpty() const noexcept {
    dbg::Assert(IsOpen());
    return GetNode().size == min_entry_count * sizeof(DirEntry);
}

bool Directory::IsOpen() const noexcept {
    return inode != nullptr;
}

IdxNode& Directory::GetNode() const noexcept {
    dbg::Assert(IsOpen());
    return *inode;
}

stl::size_t Directory::GetNodeIdx() const noexcept {
    return GetNode().idx;
}

DirEntry& DirEntry::SetName(const stl::string_view name) noexcept {
    dbg::Assert(!name.empty());
    stl::strcpy_s(this->name.data(), this->name.max_size(), name.data());
    return *this;
}

DirEntry::DirEntry(const FileType type, const stl::string_view name,
                   const stl::size_t inode_idx) noexcept :
    type {type}, inode_idx {inode_idx} {
    SetName(name);
}

}  // namespace io::fs