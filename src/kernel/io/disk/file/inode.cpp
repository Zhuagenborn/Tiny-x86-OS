#include "kernel/io/disk/file/inode.h"
#include "kernel/memory/pool.h"

namespace io::fs {

IdxNode::IdxNode() noexcept {
    Init();
}

IdxNode& IdxNode::Init() noexcept {
    tag = {};
    idx = npos;
    size = 0;
    open_times = 0;
    write_deny = false;
    return *this;
}

void IdxNode::Close() noexcept {
    const intr::IntrGuard guard;
    if (--open_times == 0) {
        tag.Detach();
        mem::Free(mem::PoolType::Kernel, this);
    }
}

IdxNode& IdxNode::GetByTag(const TagList::Tag& tag) noexcept {
    return tag.GetElem<IdxNode>();
}

void IdxNode::CloneToPure(IdxNode& inode) const noexcept {
    stl::memcpy(&inode, this, sizeof(IdxNode));
    inode.open_times = 0;
    inode.write_deny = false;
    inode.tag = {};
}

bool IdxNode::IsOpen() const noexcept {
    return open_times != 0;
}

stl::size_t IdxNode::GetIndirectTabLba() const noexcept {
    return indirect_tab_lba_;
}

IdxNode& IdxNode::SetIndirectTabLba(const stl::size_t lba) noexcept {
    indirect_tab_lba_ = lba;
    return *this;
}

IdxNode& IdxNode::SetDirectLba(const stl::size_t idx, const stl::size_t lba) noexcept {
    dbg::Assert(idx < direct_block_count);
    direct_lbas_[idx] = lba;
    return *this;
}

stl::size_t IdxNode::GetDirectLba(const stl::size_t idx) const noexcept {
    dbg::Assert(idx < direct_block_count);
    return direct_lbas_[idx];
}

}  // namespace io::fs