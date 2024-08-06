/**
 * The super block.
 */

#pragma once

#include "kernel/io/disk/disk.h"
#include "kernel/stl/array.h"

namespace io::fs {

#pragma pack(push, 1)

/**
 * @brief The super block.
 *
 * @details
 * The super block is 4096 bytes in size and starts at offset @p 4096 bytes in a partition, behind the boot sector.
 * It maintains information about the entire file system.
 * ```
 *                                                     Disk
 *                                    ┌─────┬───────────┬─────┬───────────┐
 *                                    │ MBR │ Partition │ ... │ Partition │
 *                                    └─────┴───────────┴─────┴───────────┘
 *                                          │           │
 * ┌────────────────────────────────────────┘           └─────────────────────────────────────────────────┐
 * ▼                                                                                                      ▼
 * ┌─────────────┬─────────────┬──────────────┬───────────────────┬─────────────┬────────────────┬────────┐
 * │ Boot Sector │ Super Block │ Block Bitmap │ Index Node Bitmap │ Index Nodes │ Root Directory │ Blocks │
 * └─────────────┴─────────────┴──────────────┴───────────────────┴─────────────┴────────────────┴────────┘
 * ```
 */
struct SuperBlock {
private:
    static constexpr stl::uint32_t sign {0x11223344};

    //! The magic number.
    stl::uint32_t sign_ {sign};

public:
    //! The start LBA.
    static constexpr stl::uint32_t start_lba {boot_sector_count};

    /**
     * @brief Whether the super block has a valid signature.
     *
     * @details
     * This method can be used to check whether a partition has been formatted.
     */
    bool IsSignValid() const noexcept;

    //! The start LBA of the partition.
    stl::size_t part_start_lba;
    //! The number of sectors in the partition.
    stl::size_t part_sector_count;
    //! The number of index nodes in the partition.
    stl::size_t part_inode_count;

    //! The start LBA of the block bitmap area.
    stl::size_t block_bitmap_start_lba;
    //! The number of sectors in the block bitmap area.
    stl::size_t block_bitmap_sector_count;

    //! The start LBA of the index node bitmap area.
    stl::size_t inode_bitmap_start_lba;
    //! The number of sectors in the index node bitmap area.
    stl::size_t inode_bitmap_sector_count;

    //! The start LBA of the index node area.
    stl::size_t inodes_start_lba;
    //! The number of sectors in the index node area.
    stl::size_t inodes_sector_count;

    //! The start LBA of the data area.
    stl::size_t data_start_lba;
    //! The index of the root directory's index node.
    stl::size_t root_inode_idx;
};

/**
 * The 4096-byte padded super block.
 */
class PaddedSuperBlock : public SuperBlock {
public:
    /**
     * @brief Write the super block and initialization data to a disk.
     *
     * @param part A disk.
     * @param block_bitmap_bit_len The number of bits in the block bitmap.
     */
    PaddedSuperBlock& WriteTo(Disk::Part& part, stl::size_t block_bitmap_bit_len) noexcept;

private:
    /**
     * @brief Write the block bitmap to a disk.
     *
     * @param disk A disk.
     * @param bit_len The number of bits in the block bitmap.
     * @param io_buf An I/O buffer for temporary data storage.
     * @param io_buf_size The size of the I/O buffer.
     */
    PaddedSuperBlock& WriteBlockBitmap(Disk& disk, stl::size_t bit_len, void* io_buf,
                                       stl::size_t io_buf_size) noexcept;

    //! Write the index node bitmap to a disk.
    PaddedSuperBlock& WriteNodeBitmap(Disk& disk, void* io_buf, stl::size_t io_buf_size) noexcept;

    //! Write the index node of the root directory to a disk.
    PaddedSuperBlock& WriteRootDirNode(Disk& disk, void* io_buf, stl::size_t io_buf_size) noexcept;

    /**
     * Write the entries in the root directory to a disk, including:
     * - The current directory.
     * - The parent directory.
     */
    PaddedSuperBlock& WriteRootDirEntries(Disk& disk, void* io_buf,
                                          stl::size_t io_buf_size) noexcept;

    stl::array<stl::byte, Disk::sector_size - sizeof(SuperBlock)> padding_;
};

static_assert(sizeof(PaddedSuperBlock) == Disk::sector_size);

#pragma pack(pop)

}  // namespace io::fs