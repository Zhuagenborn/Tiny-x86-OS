#include "kernel/io/disk/file/super_block.h"
#include "kernel/io/disk/file/dir.h"
#include "kernel/io/disk/file/inode.h"
#include "kernel/io/file/path.h"
#include "kernel/memory/pool.h"

namespace io::fs {

namespace {
//! The number of bits in a sector.
inline constexpr stl::size_t bit_count_per_sector {Disk::sector_size * bit::byte_len};
}  // namespace

bool SuperBlock::IsSignValid() const noexcept {
    return sign_ == sign;
}

PaddedSuperBlock& PaddedSuperBlock::WriteTo(Disk::Part& part,
                                            const stl::size_t block_bitmap_bit_len) noexcept {
    // Write the super block to the disk.
    auto& disk {part.GetDisk()};
    disk.WriteSectors(part.GetStartLba() + start_lba, this,
                      RoundUpDivide<stl::size_t>(sizeof(PaddedSuperBlock), Disk::sector_size));

    // Write initialization data to the disk.
    const auto io_buf_size {stl::max(stl::max(inode_bitmap_sector_count, inodes_sector_count),
                                     block_bitmap_sector_count)
                            * Disk::sector_size};
    const auto io_buf {mem::Allocate(io_buf_size)};
    mem::AssertAlloc(io_buf);
    WriteBlockBitmap(disk, block_bitmap_bit_len, io_buf, io_buf_size);
    WriteNodeBitmap(disk, io_buf, io_buf_size);
    WriteRootDirNode(disk, io_buf, io_buf_size);
    WriteRootDirEntries(disk, io_buf, io_buf_size);
    mem::Free(io_buf);
    return *this;
}

PaddedSuperBlock& PaddedSuperBlock::WriteBlockBitmap(Disk& disk, const stl::size_t bit_len,
                                                     void* const io_buf,
                                                     const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= block_bitmap_sector_count * Disk::sector_size);
    stl::memset(io_buf, 0, io_buf_size);
    const auto round_up_bit_len {block_bitmap_sector_count * bit_count_per_sector};
    dbg::Assert(round_up_bit_len >= bit_len);
    Bitmap {io_buf, round_up_bit_len / bit::byte_len}
        // The bit for the root directory is occupied.
        .ForceAlloc(root_inode_idx)
        // There are usually some extra bits at the end of the last sector in the block bitmap.
        // They do not indicate any available blocks.
        // Marking them as occupied prevents them from being allocated improperly in the future.
        .ForceAlloc(bit_len, round_up_bit_len - bit_len);
    disk.WriteSectors(block_bitmap_start_lba, io_buf, block_bitmap_sector_count);
    return *this;
}

PaddedSuperBlock& PaddedSuperBlock::WriteNodeBitmap(Disk& disk, void* const io_buf,
                                                    const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= inode_bitmap_sector_count * Disk::sector_size);
    stl::memset(io_buf, 0, io_buf_size);
    Bitmap {io_buf, inode_bitmap_sector_count * Disk::sector_size}
        // The bit for the root directory is occupied.
        .ForceAlloc(root_inode_idx);
    disk.WriteSectors(inode_bitmap_start_lba, io_buf, inode_bitmap_sector_count);
    return *this;
}

PaddedSuperBlock& PaddedSuperBlock::WriteRootDirNode(Disk& disk, void* const io_buf,
                                                     const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= inodes_sector_count * Disk::sector_size);
    stl::memset(io_buf, 0, io_buf_size);
    const auto root {static_cast<IdxNode*>(io_buf) + root_inode_idx};
    root->idx = root_inode_idx;
    root->size = Directory::min_entry_count * sizeof(DirEntry);
    // The entries in the root directory are saved at the begging of the data area.
    root->SetDirectLba(0, data_start_lba);
    disk.WriteSectors(inodes_start_lba, io_buf, inodes_sector_count);
    return *this;
}

PaddedSuperBlock& PaddedSuperBlock::WriteRootDirEntries(Disk& disk, void* const io_buf,
                                                        const stl::size_t io_buf_size) noexcept {
    const auto sector_count {RoundUpDivide<stl::size_t>(
        Directory::min_entry_count * sizeof(DirEntry), Disk::sector_size)};
    dbg::Assert(io_buf && io_buf_size >= sector_count * Disk::sector_size);
    stl::memset(io_buf, 0, io_buf_size);

    const auto curr_dir {static_cast<DirEntry*>(io_buf)};
    curr_dir->SetName(Path::curr_dir_name);
    // The current directory is the root directory.
    curr_dir->inode_idx = root_inode_idx;
    curr_dir->type = FileType::Directory;

    const auto parent_dir {curr_dir + 1};
    parent_dir->SetName(Path::parent_dir_name);
    // The parent directory of the root directory is itself.
    parent_dir->inode_idx = root_inode_idx;
    parent_dir->type = FileType::Directory;

    // Write entries to the begging of the data area.
    disk.WriteSectors(data_start_lba, io_buf, sector_count);
    return *this;
}

}  // namespace io::fs