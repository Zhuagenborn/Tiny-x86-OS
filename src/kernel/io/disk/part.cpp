#include "kernel/interrupt/intr.h"
#include "kernel/io/disk/disk.h"
#include "kernel/io/disk/file/inode.h"
#include "kernel/io/disk/file/super_block.h"
#include "kernel/io/disk/ide.h"
#include "kernel/io/video/print.h"
#include "kernel/memory/pool.h"

namespace io {

static_assert(sizeof(fs::IdxNode) < Disk::sector_size);

namespace {

//! The maximum number of files in a partition.
inline constexpr stl::size_t max_file_count_per_part {0x1000};

//! The number of bits in a sector.
inline constexpr stl::size_t bit_count_per_sector {Disk::sector_size * bit::byte_len};

//! The number of directory entries in a sector.
inline constexpr stl::size_t dir_entry_count_per_sector {Disk::sector_size / sizeof(fs::DirEntry)};

//! The number of sectors used by a single indirect block table.
inline constexpr stl::size_t indirect_tab_sector_count_per_inode {1};

/**
 * @brief The number of sectors in an index node's indirect blocks.
 *
 * @details
 * In our system, a block is a sector in an index node.
 */
inline constexpr stl::size_t indirect_sector_count_per_inode {
    indirect_tab_sector_count_per_inode * Disk::sector_size / sizeof(stl::size_t)};

/**
 * @brief The number of sectors in an index node's blocks, including both direct and indirect blocks.
 *
 * @details
 * In our system, a block is a sector in an index node.
 */
inline constexpr stl::size_t sector_count_per_inode {fs::IdxNode::direct_block_count
                                                     + indirect_sector_count_per_inode};

//! The position of an index node in a partition.
struct IdxNodePos {
    IdxNodePos(const Disk::FilePart& part, const fs::IdxNode& inode) noexcept :
        IdxNodePos {part, inode.idx} {}

    IdxNodePos(const Disk::FilePart& part, const stl::size_t idx) noexcept {
        dbg::Assert(idx < max_file_count_per_part);
        const auto offset {idx * sizeof(fs::IdxNode)};
        offset_in_sector = offset % Disk::sector_size;
        lba = part.GetSuperBlock().inodes_start_lba + offset / Disk::sector_size;
        dbg::Assert(lba < part.GetStartLba() + part.GetSectorCount());
        is_across_sectors = Disk::sector_size - offset_in_sector < sizeof(fs::IdxNode);
    }

    //! Whether the index node is across two sectors.
    bool is_across_sectors;

    //! The sector LBA.
    stl::size_t lba;

    //! The offset in the sector.
    stl::size_t offset_in_sector;
};

//! Load an index node's all block LBAs, including both direct and indirect blocks.
stl::array<stl::size_t, sector_count_per_inode> LoadNodeLbas(const Disk& disk,
                                                             const fs::IdxNode& inode) noexcept {
    stl::array<stl::size_t, sector_count_per_inode> lbas;

    // Load direct blocks.
    for (stl::size_t i {0}; i != fs::IdxNode::direct_block_count; ++i) {
        lbas[i] = inode.GetDirectLba(i);
    }

    // Load indirect blocks.
    if (const auto indirect_tab_lba {inode.GetIndirectTabLba()}; indirect_tab_lba != 0) {
        disk.ReadSectors(indirect_tab_lba, lbas.data() + fs::IdxNode::direct_block_count,
                         indirect_tab_sector_count_per_inode);
    }

    return lbas;
}

/**
 * @brief Calculate the number of sectors used by the block bitmap.
 *
 * @details
 * The number of blocks in a partition and the size of the block bitmap influence each other.
 * We use a simple calculation, but it is inaccurate and causes some memory waste.
 *
 * @param free_sector_count The number of free sectors.
 * @param[out] block_bitmap_bit_len The number of bits in the block bitmap.
 * @return The number of sectors in the block bitmap.
 */
stl::size_t CalcSectorCountForBlockBitmap(const stl::size_t free_sector_count,
                                          stl::size_t& block_bitmap_bit_len) noexcept {
    // Calculate the number of sectors needed for free blocks in the block bitmap
    // Each free block needs a bit.
    const auto block_bitmap_sector_count {RoundUpDivide(free_sector_count, bit_count_per_sector)};
    // The remaining blocks can be used by users.
    block_bitmap_bit_len = free_sector_count - block_bitmap_sector_count;
    return RoundUpDivide(block_bitmap_bit_len, bit_count_per_sector);
}

//! Format a partition and create a file system in it.
void FormatPart(Disk::Part& part) noexcept {
    // Calculate the numbers of sectors for the super block, index nodes and bitmaps.
    constexpr stl::size_t super_block_sector_count {
        RoundUpDivide<stl::size_t>(sizeof(fs::PaddedSuperBlock), Disk::sector_size)};
    dbg::Assert(max_file_count_per_part % bit_count_per_sector == 0);
    constexpr auto inode_bitmap_sector_count {max_file_count_per_part / bit_count_per_sector};
    const auto inodes_sector_count {RoundUpDivide<stl::size_t>(
        max_file_count_per_part * sizeof(fs::IdxNode), Disk::sector_size)};
    const auto used_sector_count {boot_sector_count + super_block_sector_count
                                  + inode_bitmap_sector_count + inodes_sector_count};
    const auto free_sector_count {part.GetSectorCount() - used_sector_count};
    stl::size_t block_bitmap_bit_len {0};
    const auto block_bitmap_sector_count {
        CalcSectorCountForBlockBitmap(free_sector_count, block_bitmap_bit_len)};

    // Initialize the super block and write it to the partition.
    fs::PaddedSuperBlock super_block {};
    super_block.part_inode_count = max_file_count_per_part;
    super_block.part_sector_count = part.GetSectorCount();
    super_block.part_start_lba = part.GetStartLba();

    super_block.block_bitmap_sector_count = block_bitmap_sector_count;
    super_block.block_bitmap_start_lba =
        super_block.part_start_lba + fs::PaddedSuperBlock::start_lba + super_block_sector_count;

    super_block.inode_bitmap_sector_count = inode_bitmap_sector_count;
    super_block.inode_bitmap_start_lba =
        super_block.block_bitmap_start_lba + super_block.block_bitmap_sector_count;

    super_block.inodes_sector_count = inodes_sector_count;
    super_block.inodes_start_lba =
        super_block.inode_bitmap_start_lba + super_block.inode_bitmap_sector_count;

    super_block.data_start_lba = super_block.inodes_start_lba + super_block.inodes_sector_count;
    super_block.root_inode_idx = fs::root_inode_idx;

    super_block.WriteTo(part, block_bitmap_bit_len);
}

/**
 * @brief A wrapper of a global variable representing the default partition.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
Disk::FilePart*& GetDefaultPartImpl() noexcept {
    static Disk::FilePart* part {nullptr};
    return part;
}

//! Load all entries of a directory from a sector.
stl::array<fs::DirEntry, dir_entry_count_per_sector> LoadDirEntries(
    const Disk& disk, const stl::size_t lba) noexcept {
    stl::array<fs::DirEntry, dir_entry_count_per_sector + 1> padded_entries;
    disk.ReadSectors(lba, padded_entries.data());
    stl::array<fs::DirEntry, dir_entry_count_per_sector> entries;
    stl::memcpy(entries.data(), padded_entries.data(), entries.size() * sizeof(fs::DirEntry));
    return entries;
}

//! Load all entries of a directory from a sector.
fs::DirEntry* LoadDirEntries(const Disk& disk, const stl::size_t lba, void* const buf,
                             const stl::size_t buf_size) noexcept {
    dbg::Assert(buf && buf_size >= Disk::sector_size);
    disk.ReadSectors(lba, buf);
    return static_cast<fs::DirEntry*>(buf);
}

/**
 * @brief Mount the default partition.
 *
 * @details
 * All file and directory operations use the default partition as the target.
 */
void MountDefaultPart() noexcept {
    constexpr stl::string_view default_part {"sdb1"};
    GetDiskParts().Find(
        [](const TagList::Tag& part_tag, void* const name) noexcept {
            auto& part {Disk::FilePart::GetByTag(part_tag)};
            if (part.GetName() == static_cast<const char*>(name)) {
                GetDefaultPartImpl() = &part;
                part.LoadSuperBlock();
                io::Printf("The partition '{}' has been mounted.\n", part.GetName());
                return true;
            } else {
                return false;
            }
        },
        const_cast<char*>(default_part.data()));

    if (!GetDefaultPartImpl()) {
        io::Printf("Failed to find the default mount partition '{}'.\n", default_part);
        dbg::Assert(false);
    }
}

}  // namespace

stl::string_view Disk::Part::GetName() const noexcept {
    return name_.data();
}

stl::size_t Disk::Part::GetStartLba() const noexcept {
    return start_lba_;
}

Disk::Part& Disk::Part::SetName(const stl::string_view name) noexcept {
    if (name.empty()) {
        stl::memset(name_.data(), '\0', name_.size());
    } else {
        stl::strcpy_s(name_.data(), name_.size(), name.data());
    }

    return *this;
}

stl::size_t Disk::Part::GetSectorCount() const noexcept {
    return sector_count_;
}

const Disk& Disk::Part::GetDisk() const noexcept {
    dbg::Assert(disk_);
    return *disk_;
}

Disk& Disk::Part::GetDisk() noexcept {
    return const_cast<Disk&>(const_cast<const Part&>(*this).GetDisk());
}

bool Disk::Part::IsValid() const noexcept {
    return disk_ != nullptr && sector_count_ > 0;
}

TagList::Tag& Disk::Part::GetTag() noexcept {
    return tag_;
}

const TagList::Tag& Disk::Part::GetTag() const noexcept {
    return const_cast<TagList::Tag&>(const_cast<const Part&>(*this).GetTag());
}

Disk::Part& Disk::Part::GetByTag(const TagList::Tag& tag) noexcept {
    return tag.GetElem<Part>();
}

Disk::FilePart& Disk::FilePart::GetByTag(const TagList::Tag& tag) noexcept {
    return static_cast<FilePart&>(Part::GetByTag(tag));
}

const Disk::FilePart& Disk::FilePart::LoadBlockBitmap() const noexcept {
    const auto& super_block {GetSuperBlock()};
    const auto byte_len {super_block.block_bitmap_sector_count * sector_size};
    const auto bits {mem::Allocate(byte_len)};
    mem::AssertAlloc(bits);
    GetDisk().ReadSectors(super_block.block_bitmap_start_lba, bits,
                          super_block.block_bitmap_sector_count);
    block_bitmap_.Init(bits, byte_len, false);
    return *this;
}

const Disk::FilePart& Disk::FilePart::LoadNodeBitmap() const noexcept {
    const auto& super_block {GetSuperBlock()};
    const auto byte_len {super_block.inode_bitmap_sector_count * sector_size};
    const auto bits {mem::Allocate(byte_len)};
    mem::AssertAlloc(bits);
    GetDisk().ReadSectors(super_block.inode_bitmap_start_lba, bits,
                          super_block.inode_bitmap_sector_count);
    inode_bitmap_.Init(bits, byte_len, false);
    return *this;
}

void Disk::FilePart::OpenRootDir() const noexcept {
    auto& dir {fs::GetRootDir()};
    const auto inode {dir.inode};
    dir.pos = 0;
    dir.inode = &OpenNode(GetSuperBlock().root_inode_idx);
    // The previously opened root directory may belong to another partition.
    // So we open the new directory first, then close the old one.
    if (inode) {
        inode->Close();
    }
}

bool Disk::FilePart::SearchPath(const Path& path, PathSearchRecord& record) const noexcept {
    dbg::Assert(path.IsAbsolute());
    if (path.IsRootDir()) {
        record.inode_idx = fs::root_inode_idx;
        record.parent = &fs::GetRootDir();
        record.type = fs::FileType::Directory;
        record.searched.Clear();
        return true;
    }

    struct VisitorArg {
        const Disk::FilePart* part {nullptr};
        PathSearchRecord* record {nullptr};
        fs::DirEntry entry;
        stl::size_t parent_inode_idx {fs::root_inode_idx};
    };

    // The search starts from the root directory.
    record.type = fs::FileType::Unknown;
    record.parent = &fs::GetRootDir();
    stl::array<char, Path::max_len + 1> name;
    VisitorArg arg {this, &record};
    // Iterate through each part of the path.
    path.Visit(
        [](const stl::string_view sub_path, const stl::string_view name, void* const arg) noexcept {
            const auto search {static_cast<VisitorArg*>(arg)};
            dbg::Assert(search && search->record && search->part);
            // Record the found path.
            search->record->searched.Join(name);
            dbg::Assert(search->record->parent);

            // Search the entry in the parent directory.
            if (search->part->SearchDirEntry(*search->record->parent, name, search->entry)) {
                if (search->entry.type == fs::FileType::Directory) {
                    // Record the index node ID of the previous parent directory.
                    search->parent_inode_idx = search->record->parent->GetNodeIdx();
                    search->record->type = fs::FileType::Directory;
                    // Close the previous parent directory and open the current directory as the new parent directory for the next search.
                    search->record->parent->Close();
                    search->record->parent = &search->part->OpenDir(search->record->inode_idx);
                    // Continue the search.
                    return true;
                } else if (search->entry.type == fs::FileType::Regular) {
                    // Stop the search since a subpath refers to a file instead of a directory.
                    search->record->type = fs::FileType::Regular;
                    search->record->inode_idx = search->entry.inode_idx;
                    return false;
                } else {
                    dbg::Assert(false);
                    return false;
                }
            } else {
                // Stop the search since a subpath does not exist.
                search->record->type = fs::FileType::Unknown;
                return false;
            }
        },
        &arg);

    dbg::Assert(record.parent && record.parent->IsOpen());
    if (record.type != fs::FileType::Unknown) {
        // The path is found.
        if (record.type == fs::FileType::Directory) {
            // When the found path refers to a directory such as `/a/b/c` in which `c` is a directory,
            // the last opened parent directory in the track record is `c` instead of `b`.
            // We should close it and open the direct parent directory `b`.
            record.parent->Close();
            record.parent = &OpenDir(arg.parent_inode_idx);
        }

        return true;
    } else {
        return false;
    }
}

fs::DirEntry* Disk::FilePart::ReadDir(const fs::Directory& dir) const noexcept {
    dbg::Assert(dir.IsOpen());
    if (dir.pos >= dir.GetNode().size) {
        // All entries have been read.
        return nullptr;
    }

    const auto& disk {GetDisk()};
    stl::size_t pos {0};
    for (const auto lba : LoadNodeLbas(disk, dir.GetNode())) {
        if (lba == 0) {
            continue;
        }

        for (const auto& entry : LoadDirEntries(disk, lba)) {
            if (entry.type != fs::FileType::Unknown) {
                if (pos < dir.pos) {
                    // Keep moving until reaching the current access offset.
                    pos += sizeof(fs::DirEntry);
                } else {
                    // Read the next entry and update the access offset.
                    dbg::Assert(pos == dir.pos);
                    dir.pos += sizeof(fs::DirEntry);
                    return const_cast<fs::DirEntry*>(&entry);
                }
            }
        }
    }

    return nullptr;
}

fs::Directory& Disk::FilePart::OpenDir(const stl::size_t inode_idx) const noexcept {
    const auto dir {mem::Allocate<fs::Directory>(sizeof(fs::Directory))};
    mem::AssertAlloc(dir);
    dir->pos = 0;
    dir->inode = &OpenNode(inode_idx);
    return *dir;
}

bool Disk::FilePart::SearchDirEntry(const fs::Directory& dir, const stl::string_view name,
                                    fs::DirEntry& found_entry) const noexcept {
    dbg::Assert(dir.IsOpen());
    dbg::Assert(!name.empty() && name.size() <= Path::max_len);

    const auto& disk {GetDisk()};
    auto found {false};
    for (const auto lba : LoadNodeLbas(disk, dir.GetNode())) {
        if (lba == 0) {
            continue;
        }

        for (const auto& entry : LoadDirEntries(disk, lba)) {
            if (entry.name.data() == name) {
                found_entry = entry;
                found = true;
                break;
            }
        }

        if (found) {
            break;
        }
    }

    return found;
}

Disk::FilePart& Disk::FilePart::LoadSuperBlock() noexcept {
    dbg::Assert(!super_block_);
    constexpr auto sector_count {
        RoundUpDivide<stl::size_t>(sizeof(fs::PaddedSuperBlock), sector_size)};
    const auto buf {mem::Allocate(sector_count * sector_size)};
    mem::AssertAlloc(buf);
    GetDisk().ReadSectors(start_lba_ + fs::PaddedSuperBlock::start_lba, buf, sector_count);
    super_block_ = mem::Allocate<fs::SuperBlock>(sizeof(fs::SuperBlock));
    mem::AssertAlloc(super_block_);
    stl::memcpy(super_block_, buf, sizeof(fs::SuperBlock));
    mem::Free(buf);

    LoadBlockBitmap();
    LoadNodeBitmap();
    return *this;
}

const fs::SuperBlock& Disk::FilePart::GetSuperBlock() const noexcept {
    dbg::Assert(super_block_);
    return *super_block_;
}

stl::size_t Disk::FilePart::AllocNode() const noexcept {
    if (const auto idx {inode_bitmap_.Alloc()}; idx != npos) {
        return idx;
    } else {
        io::PrintlnStr("The partition has no available index node.");
        return npos;
    }
}

const Disk::FilePart& Disk::FilePart::FreeNode(const stl::size_t idx) const noexcept {
    inode_bitmap_.Free(idx);
    return *this;
}

const Disk::FilePart& Disk::FilePart::FreeBlock(const stl::size_t lba) const noexcept {
    const auto start_lba {GetSuperBlock().data_start_lba};
    dbg::Assert(lba >= start_lba);
    block_bitmap_.Free(lba - start_lba);
    return *this;
}

stl::size_t Disk::FilePart::AllocBlock() const noexcept {
    if (const auto lba {block_bitmap_.Alloc()}; lba != npos) {
        return lba + GetSuperBlock().data_start_lba;
    } else {
        io::PrintlnStr("The partition has no available data block.");
        return npos;
    }
}

Disk::FilePart& Disk::FilePart::SyncNodeBitmap(const stl::size_t idx) noexcept {
    return SyncBitmap(BitmapType::Node, idx);
}

Disk::FilePart& Disk::FilePart::SyncBlockBitmap(const stl::size_t lba) noexcept {
    const auto start_lba {GetSuperBlock().data_start_lba};
    dbg::Assert(lba >= start_lba);
    return SyncBitmap(BitmapType::Block, lba - start_lba);
}

Disk::FilePart& Disk::FilePart::SyncBitmap(const BitmapType type,
                                           const stl::size_t bit_idx) noexcept {
    const auto sector_offset {bit_idx / bit_count_per_sector};
    const auto byte_offset {sector_offset * sector_size};

    stl::size_t lba {sector_offset};
    const void* bits {nullptr};
    switch (type) {
        case BitmapType::Node: {
            lba += GetSuperBlock().inode_bitmap_start_lba;
            bits = static_cast<const stl::byte*>(inode_bitmap_.GetBits()) + byte_offset;
            break;
        }
        case BitmapType::Block: {
            lba += GetSuperBlock().block_bitmap_start_lba;
            bits = static_cast<const stl::byte*>(block_bitmap_.GetBits()) + byte_offset;
            break;
        }
        default: {
            dbg::Assert(false);
            break;
        }
    }

    GetDisk().WriteSectors(lba, bits);
    return *this;
}

fs::IdxNode& Disk::FilePart::OpenNode(const stl::size_t idx) const noexcept {
    dbg::Assert(idx < max_file_count_per_part);
    // Try to find the index node in the list of open nodes.
    if (const auto found_tag {open_inodes_.Find(
            [](const TagList::Tag& inode_tag, void* const idx) noexcept {
                const auto& inode {fs::IdxNode::GetByTag(inode_tag)};
                return inode.idx == reinterpret_cast<stl::size_t>(idx);
            },
            reinterpret_cast<void*>(idx))};
        found_tag) {
        // The index node is already open.
        auto& found_inode {fs::IdxNode::GetByTag(*found_tag)};
        found_inode.open_times += 1;
        return found_inode;
    } else {
        // Allocate a new index node.
        const auto new_inode {
            mem::Allocate<fs::IdxNode>(mem::PoolType::Kernel, sizeof(fs::IdxNode))};
        mem::AssertAlloc(new_inode);
        new_inode->Init();

        // Read the index node data from the disk.
        const IdxNodePos pos {*this, idx};
        const auto sector_count {pos.is_across_sectors ? 2 : 1};
        const auto buf {mem::Allocate<stl::byte>(sector_count * sector_size)};
        mem::AssertAlloc(buf);
        GetDisk().ReadSectors(pos.lba, buf, sector_count);
        stl::memcpy(new_inode, buf + pos.offset_in_sector, sizeof(fs::IdxNode));
        mem::Free(buf);

        // Add the index node to the list of open nodes.
        new_inode->open_times = 1;
        open_inodes_.PushBack(new_inode->tag);
        return *new_inode;
    }
}

Disk::FilePart& Disk::FilePart::DeleteNode(const stl::size_t idx) noexcept {
    auto& inode {OpenNode(idx)};
    // Free all direct and indirect blocks.
    for (const auto lba : LoadNodeLbas(GetDisk(), inode)) {
        if (lba != 0) {
            FreeBlock(lba);
            SyncBlockBitmap(lba);
        }
    }

    // Free the single indirect block table.
    if (const auto indirect_tab_lba {inode.GetIndirectTabLba()}; indirect_tab_lba != 0) {
        FreeBlock(indirect_tab_lba);
        SyncBlockBitmap(indirect_tab_lba);
    }

    // Free the index node.
    FreeNode(idx);
    SyncNodeBitmap(idx);

    constexpr auto io_buf_size {2 * sector_size};
    if (const auto io_buf {mem::Allocate(io_buf_size)}; io_buf) {
        ZeroFillNode(idx, io_buf, io_buf_size);
        mem::Free(io_buf);
    }

    inode.Close();
    return *this;
}

Disk::FilePart& Disk::FilePart::ZeroFillNode(const stl::size_t idx, void* const io_buf,
                                             const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= 2 * sector_size);
    dbg::Assert(idx < max_file_count_per_part);
    auto& disk {GetDisk()};
    const IdxNodePos pos {*this, idx};
    const auto sector_count {pos.is_across_sectors ? 2 : 1};
    disk.ReadSectors(pos.lba, io_buf, sector_count);
    stl::memset(static_cast<stl::byte*>(io_buf) + pos.offset_in_sector, 0, sizeof(fs::IdxNode));
    disk.WriteSectors(pos.lba, io_buf, sector_count);
    return *this;
}

FileDesc Disk::FilePart::OpenFile(const stl::size_t inode_idx,
                                  const bit::Flags<File::OpenMode> flags) const noexcept {
    auto& tab {fs::GetFileTab()};
    const auto desc {tab.GetFreeDesc()};
    if (!desc.IsValid()) {
        return {};
    }

    auto& inode {OpenNode(inode_idx)};
    if (flags.IsSet(File::OpenMode::WriteOnly) || flags.IsSet(File::OpenMode::ReadWrite)) {
        const intr::IntrGuard guard;
        if (!inode.write_deny) {
            inode.write_deny = true;
        } else {
            inode.Close();
            io::PrintlnStr("The file cannot be written now.");
            return {};
        }
    }

    dbg::Assert(desc < tab.GetSize());
    tab[desc].Clear();
    tab[desc].inode = &inode;
    tab[desc].flags = flags;
    return tsk::ProcFileDescTab::SyncGlobal(desc);
}

stl::size_t Disk::FilePart::WriteFile(const FileDesc desc, const void* const data,
                                      const stl::size_t size) noexcept {
    auto& tab {fs::GetFileTab()};
    const auto idx {tsk::ProcFileDescTab::GetGlobal(desc)};
    dbg::Assert(idx < tab.GetSize());
    return WriteFile(tab[idx], data, size);
}

stl::size_t Disk::FilePart::SeekFile(const FileDesc desc, const stl::int32_t offset,
                                     const File::SeekOrigin origin) const noexcept {
    auto& tab {fs::GetFileTab()};
    const auto idx {tsk::ProcFileDescTab::GetGlobal(desc)};
    dbg::Assert(idx < tab.GetSize());
    return SeekFile(tab[idx], offset, origin);
}

stl::size_t Disk::FilePart::ReadFile(const FileDesc desc, void* const buf,
                                     const stl::size_t size) const noexcept {
    auto& tab {fs::GetFileTab()};
    const auto idx {tsk::ProcFileDescTab::GetGlobal(desc)};
    dbg::Assert(idx < tab.GetSize());
    return ReadFile(tab[idx], buf, size);
}

fs::Directory* Disk::FilePart::OpenDir(const Path& path) const noexcept {
    dbg::Assert(path.IsAbsolute());
    if (path.IsRootDir()) {
        return &fs::GetRootDir();
    }

    PathSearchRecord search;
    const auto found {SearchPath(path, search)};
    dbg::Assert(search.parent);
    if (!found) {
        io::Printf("The path '{}' does not exist.\n", search.searched.GetPath());
        return nullptr;
    } else if (search.type == fs::FileType::Regular) {
        io::Printf("The '{}' is a file.\n", search.searched.GetPath());
        return nullptr;
    }

    search.parent->Close();
    return &OpenDir(search.inode_idx);
}

stl::size_t Disk::FilePart::SeekFile(fs::File& file, const stl::int32_t offset,
                                     const File::SeekOrigin origin) const noexcept {
    const auto size {file.GetNode().size};
    switch (origin) {
        case File::SeekOrigin::Begin: {
            file.pos = stl::min<stl::size_t>(offset, size);
            break;
        }
        case File::SeekOrigin::Curr: {
            file.pos = stl::min(file.pos + offset, size);
            break;
        }
        case File::SeekOrigin::End: {
            file.pos = stl::min(size + offset, size);
            break;
        }
        default: {
            dbg::Assert(false);
            break;
        }
    }

    dbg::Assert(file.pos <= size);
    return file.pos;
}

bool Disk::FilePart::DeleteDir(fs::Directory& parent, const fs::Directory& child) noexcept {
    dbg::Assert(parent.IsOpen() && child.IsOpen());
    constexpr auto io_buf_size {2 * sector_size};
    const auto io_buf {mem::Allocate(io_buf_size)};
    mem::AssertAlloc(io_buf);
    const auto inode_idx {child.GetNodeIdx()};
    const auto success {DeleteDirEntry(parent, inode_idx, io_buf, io_buf_size)};
    if (success) {
        DeleteNode(inode_idx);
    }

    mem::Free(io_buf);
    return success;
}

stl::size_t Disk::FilePart::ReadFile(const fs::File& file, void* const buf,
                                     stl::size_t size) const noexcept {
    dbg::Assert(file.IsOpen());
    const auto& inode {file.GetNode()};
    dbg::Assert(inode.size >= file.pos);
    size = stl::min(size, inode.size - file.pos);
    if (size == 0) {
        return 0;
    }

    const auto lbas {mem::Allocate<stl::size_t>(sector_count_per_inode * sizeof(stl::size_t))};
    mem::AssertAlloc(lbas);

    constexpr auto io_buf_size {sector_size};
    const auto io_buf {mem::Allocate<stl::byte>(io_buf_size)};
    mem::AssertAlloc(io_buf);

    const auto start_sector_idx {file.pos / sector_size};
    const auto end_sector_idx {(file.pos + size) / sector_size};
    dbg::Assert(start_sector_idx <= end_sector_idx && end_sector_idx < sector_count_per_inode);

    // Collect block LBAs from which the data should be read.
    const auto& disk {GetDisk()};
    if (start_sector_idx == end_sector_idx) {
        // The required data is located in one block.
        if (end_sector_idx < fs::IdxNode::direct_block_count) {
            // The required data is located in a direct block.
            lbas[start_sector_idx] = inode.GetDirectLba(start_sector_idx);
            dbg::Assert(lbas[start_sector_idx] != 0);
        } else {
            // The required data is located in an indirect block.
            const auto indirect_tab_lba {inode.GetIndirectTabLba()};
            dbg::Assert(indirect_tab_lba != 0);
            disk.ReadSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                             indirect_tab_sector_count_per_inode);
        }
    } else {
        // The required data is located in multiple blocks.
        if (end_sector_idx < fs::IdxNode::direct_block_count) {
            // The required data is located in direct blocks.
            for (stl::size_t i {start_sector_idx}; i <= end_sector_idx; ++i) {
                lbas[i] = inode.GetDirectLba(i);
                dbg::Assert(lbas[i] != 0);
            }
        } else if (start_sector_idx < fs::IdxNode::direct_block_count
                   && end_sector_idx >= fs::IdxNode::direct_block_count) {
            // The required data is located in both direct and indirect blocks.
            for (stl::size_t i {start_sector_idx}; i != fs::IdxNode::direct_block_count; ++i) {
                lbas[i] = inode.GetDirectLba(i);
                dbg::Assert(lbas[i] != 0);
            }

            const auto indirect_tab_lba {inode.GetIndirectTabLba()};
            dbg::Assert(indirect_tab_lba != 0);
            disk.ReadSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                             indirect_tab_sector_count_per_inode);
        } else {
            // The required data is located in indirect blocks.
            const auto indirect_tab_lba {inode.GetIndirectTabLba()};
            dbg::Assert(indirect_tab_lba != 0);
            disk.ReadSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                             indirect_tab_sector_count_per_inode);
        }
    }

    // Read data from sectors and update the access offset.
    stl::size_t read_size {0};
    while (read_size < size) {
        stl::memset(io_buf, 0, io_buf_size);
        const auto sector_idx {file.pos / sector_size};
        const auto offset_in_sector {file.pos % sector_size};
        const auto left_in_sector {sector_size - offset_in_sector};
        const auto chunk_size {stl::min(size - read_size, left_in_sector)};

        disk.ReadSectors(lbas[sector_idx], io_buf);
        stl::memcpy(static_cast<stl::byte*>(buf) + read_size, io_buf + offset_in_sector,
                    chunk_size);

        read_size += chunk_size;
        file.pos += chunk_size;
    }

    mem::Free(io_buf);
    mem::Free(lbas);
    return read_size;
}

bool Disk::FilePart::CreateDir(const Path& path) noexcept {
    dbg::Assert(path.IsAbsolute());
    fs::IdxNode inode;
    fs::DirEntry entry;

    // A directory at least has two directory entries.
    constexpr auto min_sector_count_for_entries {RoundUpDivide<stl::size_t>(
        fs::Directory::min_entry_count * sizeof(fs::DirEntry), Disk::sector_size)};

    constexpr auto io_buf_size {2 * sector_size};
    dbg::Assert(io_buf_size >= min_sector_count_for_entries * Disk::sector_size);
    const auto io_buf {mem::Allocate(io_buf_size)};
    mem::AssertAlloc(io_buf);

    // The `.` entry, representing the current directory.
    const auto curr_dir_entry {static_cast<fs::DirEntry*>(io_buf)};
    // The `..` entry, representing the parent directory.
    const auto parent_dir_entry {curr_dir_entry + 1};

    PathSearchRecord search;
    const auto found {SearchPath(path, search)};
    dbg::Assert(search.parent);
    const auto name {search.searched.GetFileName()};
    const auto inode_idx {AllocNode()};
    const auto sector_lba {AllocBlock()};
    if (found) {
        io::Printf("The file or directory '{}' already exists.\n", path.GetPath());
        goto err;
    } else if (path.GetDepth() != search.searched.GetDepth()) {
        io::Printf("The path '{}' does not exist.\n", search.searched.GetPath());
        goto err;
    } else if (inode_idx == npos || sector_lba == npos) {
        goto err;
    }

    // Create a directory entry in its parent directory.
    entry.SetName(name);
    entry.type = fs::FileType::Directory;
    entry.inode_idx = inode_idx;
    if (!SyncDirEntry(*search.parent, entry, io_buf, io_buf_size)) {
        goto err;
    }

    // Update the index node of its parent directory.
    SyncNode(search.parent->GetNode(), io_buf, io_buf_size);

    // Create directory entries in the new directory.
    curr_dir_entry->SetName(Path::curr_dir_name);
    curr_dir_entry->inode_idx = inode_idx;
    curr_dir_entry->type = fs::FileType::Directory;

    parent_dir_entry->SetName(Path::parent_dir_name);
    parent_dir_entry->inode_idx = search.parent->GetNodeIdx();
    parent_dir_entry->type = fs::FileType::Directory;

    GetDisk().WriteSectors(sector_lba, io_buf, min_sector_count_for_entries);
    SyncBlockBitmap(sector_lba);

    // Create an index node for the new directory.
    inode.idx = inode_idx;
    inode.SetDirectLba(0, sector_lba);
    inode.size = fs::Directory::min_entry_count * sizeof(fs::DirEntry);

    // Save the index node of the new directory.
    SyncNode(inode, io_buf, io_buf_size);
    SyncNodeBitmap(inode_idx);

    mem::Free(io_buf);
    search.parent->Close();
    return true;

err:
    mem::Free(io_buf);

    if (inode_idx != npos) {
        FreeNode(inode_idx);
    }

    if (sector_lba != npos) {
        FreeBlock(sector_lba);
    }

    search.parent->Close();
    return false;
}

bool Disk::FilePart::DeleteFile(const Path& path) noexcept {
    dbg::Assert(path.IsAbsolute());
    if (path.IsDir()) {
        io::Printf("The path '{}' is not a file but a directory.\n", path.GetPath());
        return false;
    }

    PathSearchRecord search;
    const auto found {SearchPath(path, search)};
    dbg::Assert(search.parent);
    if (!found) {
        io::Printf("The file '{}' does not exist.\n", path.GetPath());
        search.parent->Close();
        return false;
    } else if (search.type == fs::FileType::Directory) {
        io::Printf("The path '{}' is not a file but a directory.\n", path.GetPath());
        search.parent->Close();
        return false;
    } else if (fs::GetFileTab().Contain(search.inode_idx)) {
        io::Printf("The file '{}' is in use.\n", path.GetPath());
        search.parent->Close();
        return false;
    }

    constexpr auto io_buf_size {2 * sector_size};
    const auto io_buf {mem::Allocate<stl::byte>(io_buf_size)};
    mem::AssertAlloc(io_buf);
    DeleteDirEntry(*search.parent, search.inode_idx, io_buf, io_buf_size);
    DeleteNode(search.inode_idx);
    mem::Free(io_buf);
    search.parent->Close();
    return true;
}

stl::size_t Disk::FilePart::WriteFile(fs::File& file, const void* const data,
                                      const stl::size_t size) noexcept {
    dbg::Assert(file.IsOpen());
    auto& inode {file.GetNode()};
    const auto curr_size {inode.size};
    if (curr_size + size > sector_count_per_inode * sector_size) {
        io::PrintlnStr("Failed to write. The file exceeds the maximum size.");
        return 0;
    }

    const auto lbas {mem::Allocate<stl::size_t>(sector_count_per_inode * sizeof(stl::size_t))};
    mem::AssertAlloc(lbas);

    constexpr auto io_buf_size {2 * sector_size};
    const auto io_buf {mem::Allocate<stl::byte>(io_buf_size)};
    mem::AssertAlloc(io_buf);

    const auto curr_sector_count {RoundUpDivide(curr_size, sector_size)};
    const auto new_sector_count {RoundUpDivide(curr_size + size, sector_size)};
    dbg::Assert(curr_sector_count <= new_sector_count
                && new_sector_count <= sector_count_per_inode);

    // Collect block LBAs to which the data should be written.
    auto& disk {GetDisk()};
    if (curr_sector_count == new_sector_count) {
        // The new data can be saved in the current last sector.
        if (new_sector_count <= fs::IdxNode::direct_block_count) {
            // The last sector is a direct block.
            const auto last_sector_idx {curr_size / sector_size};
            lbas[last_sector_idx] = inode.GetDirectLba(last_sector_idx);
            dbg::Assert(lbas[last_sector_idx] != 0);
        } else {
            // The last sector is an indirect block.
            const auto indirect_tab_lba {inode.GetIndirectTabLba()};
            dbg::Assert(indirect_tab_lba != 0);
            disk.ReadSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                             indirect_tab_sector_count_per_inode);
        }
    } else {
        if (new_sector_count <= fs::IdxNode::direct_block_count) {
            // The new data will be saved in direct blocks.
            if (curr_size % sector_size != 0) {
                // The last sector is not full.
                const auto last_sector_idx {curr_size / sector_size};
                lbas[last_sector_idx] = inode.GetDirectLba(last_sector_idx);
                dbg::Assert(lbas[last_sector_idx] != 0);
            }

            // Allocate new direct blocks for writting.
            auto failed {false};
            for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                dbg::Assert(lbas[i] == 0);
                if (const auto new_sector_lba {AllocBlock()}; new_sector_lba != npos) {
                    dbg::Assert(inode.GetDirectLba(i) == 0);
                    inode.SetDirectLba(i, new_sector_lba);
                    dbg::Assert(lbas[i] == 0);
                    lbas[i] = new_sector_lba;
                    SyncBlockBitmap(new_sector_lba);
                } else {
                    failed = true;
                    break;
                }
            }

            if (failed) {
                for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                    if (const auto lba {inode.GetDirectLba(i)}; lba != 0) {
                        inode.SetDirectLba(i, 0);
                        FreeBlock(lba);
                        SyncBlockBitmap(lba);
                    } else {
                        break;
                    }
                }

                mem::Free(io_buf);
                mem::Free(lbas);
                return 0;
            }

        } else if (curr_sector_count <= fs::IdxNode::direct_block_count
                   && new_sector_count > fs::IdxNode::direct_block_count) {
            // The new data will be saved in both direct and indirect blocks.
            if (curr_size % sector_size != 0) {
                // The last sector is not full.
                const auto last_sector_idx {curr_size / sector_size};
                lbas[last_sector_idx] = inode.GetDirectLba(last_sector_idx);
                dbg::Assert(lbas[last_sector_idx] != 0);
            }

            // Create a single indirect block table.
            const auto indirect_tab_lba {AllocBlock()};
            if (indirect_tab_lba == npos) {
                return 0;
            }

            dbg::Assert(inode.GetIndirectTabLba() == 0);
            inode.SetIndirectTabLba(indirect_tab_lba);

            // Allocate new direct and indirect blocks for writting.
            auto failed {false};
            for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                if (const auto new_sector_lba {AllocBlock()}; new_sector_lba != npos) {
                    if (i < fs::IdxNode::direct_block_count) {
                        dbg::Assert(inode.GetDirectLba(i) == 0);
                        inode.SetDirectLba(i, new_sector_lba);
                    }

                    dbg::Assert(lbas[i] == 0);
                    lbas[i] = new_sector_lba;
                    SyncBlockBitmap(new_sector_lba);
                } else {
                    failed = true;
                    break;
                }
            }

            if (!failed) {
                // Save indirect block LBAs to the single indirect block table.
                disk.WriteSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                                  indirect_tab_sector_count_per_inode);

            } else {
                for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                    if (lbas[i] != 0) {
                        if (i < fs::IdxNode::direct_block_count) {
                            dbg::Assert(inode.GetDirectLba(i) != 0);
                            inode.SetDirectLba(i, 0);
                        }

                        FreeBlock(lbas[i]);
                        SyncBlockBitmap(lbas[i]);
                    } else {
                        break;
                    }
                }

                dbg::Assert(inode.GetIndirectTabLba() != 0);
                inode.SetIndirectTabLba(0);
                FreeBlock(indirect_tab_lba);
                SyncBlockBitmap(indirect_tab_lba);

                mem::Free(io_buf);
                mem::Free(lbas);
                return 0;
            }

        } else {
            // The new data will be saved in indirect blocks.
            const auto indirect_tab_lba {inode.GetIndirectTabLba()};
            dbg::Assert(indirect_tab_lba != 0);
            disk.ReadSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                             indirect_tab_sector_count_per_inode);

            // Allocate new indirect blocks for writting.
            auto failed {false};
            for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                if (const auto new_sector_lba {AllocBlock()}; new_sector_lba != npos) {
                    dbg::Assert(lbas[i] == 0);
                    lbas[i] = new_sector_lba;
                    SyncBlockBitmap(new_sector_lba);
                } else {
                    failed = true;
                    break;
                }
            }

            if (!failed) {
                // Save indirect block LBAs to the single indirect block table.
                disk.WriteSectors(indirect_tab_lba, lbas + fs::IdxNode::direct_block_count,
                                  indirect_tab_sector_count_per_inode);

            } else {
                for (stl::size_t i {curr_sector_count}; i != new_sector_count; ++i) {
                    if (lbas[i] != 0) {
                        FreeBlock(lbas[i]);
                        SyncBlockBitmap(lbas[i]);
                    } else {
                        break;
                    }
                }

                mem::Free(io_buf);
                mem::Free(lbas);
                return 0;
            }
        }
    }

    // Write data to sectors and update the access offset.
    file.pos = curr_size - 1;
    auto is_first_write {true};
    stl::size_t written_size {0};
    while (written_size < size) {
        stl::memset(io_buf, 0, io_buf_size);
        const auto sector_idx {inode.size / sector_size};
        const auto offset_in_sector {inode.size % sector_size};
        const auto left_in_sector {sector_size - offset_in_sector};
        const auto chunk_size {stl::min(size - written_size, left_in_sector)};
        if (is_first_write) {
            // When new data is written to a sector for the first time, there usually exist old data in the target sector.
            // We need to read them first, then save the new data to the free area,
            // and finally write the new data to the disk together with the old data in the sector.
            disk.ReadSectors(lbas[sector_idx], io_buf);
            is_first_write = false;
        }

        stl::memcpy(io_buf + offset_in_sector, static_cast<const stl::byte*>(data) + written_size,
                    chunk_size);
        disk.WriteSectors(lbas[sector_idx], io_buf);
        written_size += chunk_size;
        file.pos += chunk_size;
        inode.size += chunk_size;
    }

    SyncNode(inode, io_buf, io_buf_size);
    mem::Free(io_buf);
    mem::Free(lbas);
    return written_size;
}

FileDesc Disk::FilePart::OpenFile(const Path& path,
                                  const bit::Flags<File::OpenMode> flags) noexcept {
    dbg::Assert(path.IsAbsolute());
    if (path.IsDir()) {
        io::Printf("The path '{}' is not a file but a directory.\n", path.GetPath());
        return {};
    }

    PathSearchRecord search;
    const auto found {SearchPath(path, search)};
    dbg::Assert(search.parent);

    auto failed {false};
    if (search.type == fs::FileType::Directory) {
        io::Printf("The path '{} is not a file but a directory.\n", path.GetPath());
        failed = true;
    } else if (path.GetDepth() != search.searched.GetDepth()) {
        io::Printf("The path '{}' does not exist.\n", search.searched.GetPath());
        failed = true;
    } else if (!found && !flags.IsSet(File::OpenMode::CreateNew)) {
        io::Printf("The file '{}' does not exist.\n", path.GetPath());
        failed = true;
    } else if (found && flags.IsSet(File::OpenMode::CreateNew)) {
        io::Printf("The file '{}' already exists.\n", path.GetPath());
        failed = true;
    }

    FileDesc desc;
    if (!failed) {
        desc = found ? OpenFile(search.inode_idx, flags)
                     : CreateFile(*search.parent, path.GetFileName(), flags);
    }

    search.parent->Close();
    return desc;
}

FileDesc Disk::FilePart::CreateFile(fs::Directory& dir, const stl::string_view name,
                                    const bit::Flags<File::OpenMode> flags) noexcept {
    dbg::Assert(dir.IsOpen());
    auto& tab {fs::GetFileTab()};
    fs::DirEntry entry;

    constexpr auto io_buf_size {2 * sector_size};
    const auto io_buf {mem::Allocate(io_buf_size)};
    mem::AssertAlloc(io_buf);

    const auto inode {mem::Allocate<fs::IdxNode>(sizeof(fs::IdxNode))};
    mem::AssertAlloc(inode);
    const auto inode_idx {AllocNode()};
    const auto desc {tab.GetFreeDesc()};
    if (inode_idx == npos || !desc.IsValid()) {
        goto rollback;
    }

    // Create an index node for the new file.
    inode->Init();
    inode->idx = inode_idx;

    dbg::Assert(desc < tab.GetSize());
    tab[desc].Clear();
    tab[desc].inode = inode;
    tab[desc].flags = flags;

    // Create a directory entry in its parent directory.
    entry.SetName(name);
    entry.type = fs::FileType::Regular;
    entry.inode_idx = inode_idx;
    if (!SyncDirEntry(dir, entry, io_buf, io_buf_size)) {
        goto rollback;
    }

    // Update the index node of its parent directory.
    SyncNode(dir.GetNode(), io_buf, io_buf_size);

    // Save the index node of the new file.
    SyncNode(*inode, io_buf, io_buf_size);
    SyncNodeBitmap(inode_idx);

    // Add the index node to the list of open nodes.
    inode->open_times = 1;
    open_inodes_.PushBack(inode->tag);

    mem::Free(io_buf);
    return tsk::ProcFileDescTab::SyncGlobal(desc);

rollback:
    mem::Free(io_buf);

    if (!inode) {
        mem::Free(inode);
    }

    if (desc.IsValid()) {
        tab[desc].Clear();
    }

    if (inode_idx != npos) {
        FreeNode(inode_idx);
    }

    return {};
}

bool Disk::FilePart::DeleteDirEntry(fs::Directory& dir, const stl::size_t inode_idx,
                                    void* const io_buf, const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= 2 * sector_size);
    dbg::Assert(dir.IsOpen());
    auto& inode {dir.GetNode()};
    dbg::Assert(inode.size >= fs::Directory::min_entry_count * sizeof(fs::DirEntry)
                && inode.size % sizeof(fs::DirEntry) == 0);
    auto& disk {GetDisk()};
    auto lbas {LoadNodeLbas(disk, inode)};
    for (stl::size_t i {0}; i != lbas.size(); ++i) {
        if (lbas[i] == 0) {
            continue;
        }

        stl::memset(io_buf, 0, io_buf_size);
        const auto entries {LoadDirEntries(disk, lbas[i], io_buf, io_buf_size)};

        fs::DirEntry* found_entry {nullptr};
        stl::size_t entry_count {0};
        // Try to find the target entry and count the number of entries in the parent directory.
        for (stl::size_t j {0}; j != dir_entry_count_per_sector; ++j) {
            if (auto& entry {entries[j]}; entry.type != fs::FileType::Unknown) {
                ++entry_count;
                if (entry.name.data() != Path::curr_dir_name
                    && entry.name.data() != Path::parent_dir_name && entry.inode_idx == inode_idx) {
                    dbg::Assert(!found_entry);
                    found_entry = &entry;
                }
            }
        }

        dbg::Assert(entry_count >= fs::Directory::min_entry_count);
        if (found_entry) {
            // The entry is found.
            if (entry_count == fs::Directory::min_entry_count + 1) {
                // The entry is the last one in the parent directory.
                // The block should be freed after deletion.
                FreeBlock(lbas[i]);
                SyncBlockBitmap(lbas[i]);
                if (i < fs::IdxNode::direct_block_count) {
                    inode.SetDirectLba(i, 0);
                } else {
                    stl::size_t indirect_block_count {0};
                    for (stl::size_t j {fs::IdxNode::direct_block_count}; j != lbas.size(); ++j) {
                        if (lbas[j] != 0) {
                            ++indirect_block_count;
                        }
                    }

                    dbg::Assert(indirect_block_count > 0);
                    const auto indirect_tab_lba {inode.GetIndirectTabLba()};
                    if (indirect_block_count > 1) {
                        lbas[i] = 0;
                        disk.WriteSectors(indirect_tab_lba,
                                          lbas.data() + fs::IdxNode::direct_block_count,
                                          indirect_tab_sector_count_per_inode);
                    } else {
                        // The block is the last one in the index node of the parent directory.
                        // Free the single indirect block table.
                        FreeBlock(indirect_tab_lba);
                        SyncBlockBitmap(indirect_tab_lba);
                        inode.SetIndirectTabLba(0);
                    }
                }
            } else {
                // Clear the entry.
                stl::memset(found_entry, 0, sizeof(fs::DirEntry));
                disk.WriteSectors(lbas[i], io_buf);
            }

            // Update the index node of the parent directory.
            inode.size -= sizeof(fs::DirEntry);
            SyncNode(inode, io_buf, io_buf_size);
            return true;
        }
    }

    return false;
}

bool Disk::FilePart::SyncDirEntry(fs::Directory& dir, const fs::DirEntry& entry, void* const io_buf,
                                  const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= sector_size);
    dbg::Assert(dir.IsOpen());
    auto& inode {dir.GetNode()};
    dbg::Assert(inode.size >= fs::Directory::min_entry_count * sizeof(fs::DirEntry)
                && inode.size % sizeof(fs::DirEntry) == 0);
    auto& disk {GetDisk()};
    auto lbas {LoadNodeLbas(disk, inode)};
    for (stl::size_t i {0}; i != lbas.size(); ++i) {
        if (lbas[i] == 0) {
            // Allocate a new block for the new directory entry.
            const auto new_sector_lba {AllocBlock()};
            if (new_sector_lba == npos) {
                return false;
            }

            lbas[i] = new_sector_lba;
            SyncBlockBitmap(new_sector_lba);
            if (i < fs::IdxNode::direct_block_count) {
                // The new block can be saved in the direct block list.
                inode.SetDirectLba(i, new_sector_lba);
            } else {
                // The new block can only be saved in the single indirect block table.
                const auto indirect_tab_lba {inode.GetIndirectTabLba()};
                if (indirect_tab_lba == 0) {
                    // Create a new single indirect block table.
                    if (const auto indirect_tab_lba {AllocBlock()}; indirect_tab_lba != npos) {
                        inode.SetIndirectTabLba(indirect_tab_lba);
                        SyncBlockBitmap(indirect_tab_lba);
                    } else {
                        FreeBlock(new_sector_lba);
                        SyncBlockBitmap(new_sector_lba);
                        return false;
                    }
                }

                // Save the new indirect block LBA to the partition.
                disk.WriteSectors(indirect_tab_lba, lbas.data() + fs::IdxNode::direct_block_count,
                                  indirect_tab_sector_count_per_inode);
            }

            // Save the new directory entry to the partition.
            stl::memset(io_buf, 0, io_buf_size);
            stl::memcpy(io_buf, &entry, sizeof(fs::DirEntry));
            disk.WriteSectors(new_sector_lba, io_buf);
            inode.size += sizeof(fs::DirEntry);
            return true;
        } else {
            // Find an empty position in an existing block for the new directory entry.
            const auto entries {LoadDirEntries(disk, lbas[i], io_buf, io_buf_size)};
            for (stl::size_t j {0}; j != dir_entry_count_per_sector; ++j) {
                if (entries[j].type == fs::FileType::Unknown) {
                    // Save the new directory entry to the partition.
                    stl::memcpy(&entries[j], &entry, sizeof(fs::DirEntry));
                    disk.WriteSectors(lbas[i], io_buf);
                    inode.size += sizeof(fs::DirEntry);
                    return true;
                }
            }
        }
    }

    io::PrintlnStr("The directory is full.");
    return false;
}

Disk::FilePart& Disk::FilePart::SyncNode(const fs::IdxNode& inode, void* const io_buf,
                                         const stl::size_t io_buf_size) noexcept {
    dbg::Assert(io_buf && io_buf_size >= 2 * sector_size);
    fs::IdxNode pure {};
    inode.CloneToPure(pure);

    auto& disk {GetDisk()};
    // Get the position of the index node.
    const IdxNodePos pos {*this, inode.idx};
    const auto sector_count {pos.is_across_sectors ? 2 : 1};
    // Read one or two sectors where the index node is located.
    disk.ReadSectors(pos.lba, io_buf, sector_count);
    // Overwrite the index node to the disk.
    stl::memcpy(static_cast<stl::byte*>(io_buf) + pos.offset_in_sector, &pure, sizeof(fs::IdxNode));
    disk.WriteSectors(pos.lba, io_buf, sector_count);
    return *this;
}

Disk::FilePart& GetDefaultPart() noexcept {
    const auto part {GetDefaultPartImpl()};
    dbg::Assert(part);
    return *part;
}

void InitFileSys() noexcept {
    dbg::Assert(IsDiskInited());
    dbg::Assert(mem::IsMemInited());
    dbg::Assert(!GetDefaultPartImpl());

    const auto super_block {mem::Allocate<fs::SuperBlock>(sizeof(fs::PaddedSuperBlock))};
    mem::AssertAlloc(super_block);

    stl::size_t inited_disk_count {0};
    // Create a file system in each partition of all disks on IDE channels.
    for (stl::size_t chnl_idx {0}; chnl_idx != GetIdeChnlCount(); ++chnl_idx) {
        auto& chnl {GetIdeChnls()[chnl_idx]};
        for (stl::size_t disk_idx {0};
             disk_idx != IdeChnl::max_disk_count && inited_disk_count != GetDiskCount();
             ++disk_idx, ++inited_disk_count) {
            // Skip the boot disk.
            if (disk_idx != boot_disk_idx) {
                auto& disk {chnl.GetDisk(disk_idx)};
                for (stl::size_t part_idx {0};
                     part_idx != Disk::prim_part_count + Disk::max_logic_part_count; ++part_idx) {
                    if (const auto part {
                            part_idx < Disk::prim_part_count
                                ? &disk.GetPrimaryPart(part_idx)
                                : &disk.GetLogicPart(part_idx - Disk::prim_part_count)};
                        part->IsValid()) {
                        // Create a super block in the partition.
                        stl::memset(super_block, 0, sizeof(fs::PaddedSuperBlock));
                        disk.ReadSectors(part->GetStartLba() + fs::PaddedSuperBlock::start_lba,
                                         super_block,
                                         RoundUpDivide<stl::size_t>(sizeof(fs::PaddedSuperBlock),
                                                                    Disk::sector_size));
                        if (!super_block->IsSignValid()) {
                            // Create a file system in the partition.
                            FormatPart(*part);
                            io::Printf("The file system on the partition '{}' has been "
                                       "formatted.\n",
                                       part->GetName());
                        } else {
                            io::Printf("The partition '{}' already has a file system.\n",
                                       part->GetName());
                        }
                    }
                }
            }
        }
    }

    // Mount the default partition and open its root directory.
    MountDefaultPart();
    GetDefaultPart().OpenRootDir();
}

namespace fs {

Directory& GetRootDir() noexcept {
    static Directory dir;
    return dir;
}

}  // namespace fs

}  // namespace io