/**
 * The disk.
 */

#pragma once

#include "kernel/io/disk/file/dir.h"
#include "kernel/io/disk/file/file.h"
#include "kernel/stl/array.h"
#include "kernel/stl/string_view.h"
#include "kernel/util/bit.h"
#include "kernel/util/bitmap.h"
#include "kernel/util/metric.h"
#include "kernel/util/tag_list.h"

namespace io {

class IdeChnl;

namespace fs {
class SuperBlock;
class IdxNode;
class Directory;
class DirEntry;
}  // namespace fs

class Disk {
public:
    //! The sector size.
    static constexpr stl::size_t sector_size {512};
    //! The number of primary partitions.
    static constexpr stl::size_t prim_part_count {4};
    //! The maximum supported number of logical partitions.
    static constexpr stl::size_t max_logic_part_count {8};
    //! The maximum supported disk size.
    static constexpr stl::size_t max_size {MB(80)};
    //! The maximum LBA.
    static constexpr stl::size_t max_lba {max_size / sector_size - 1};

    /**
     * The partition.
     */
    class Part {
        friend class Disk;

    public:
        static Part& GetByTag(const TagList::Tag&) noexcept;

        Part() noexcept = default;

        Part(const Part&) = delete;

        const TagList::Tag& GetTag() const noexcept;

        TagList::Tag& GetTag() noexcept;

        stl::string_view GetName() const noexcept;

        Part& SetName(stl::string_view) noexcept;

        stl::size_t GetStartLba() const noexcept;

        stl::size_t GetSectorCount() const noexcept;

        const Disk& GetDisk() const noexcept;

        Disk& GetDisk() noexcept;

        bool IsValid() const noexcept;

    protected:
        static constexpr stl::size_t name_len {8};

        TagList::Tag tag_;
        stl::size_t start_lba_ {0};
        stl::size_t sector_count_ {0};
        stl::array<char, name_len + 1> name_;
        Disk* disk_ {nullptr};
    };

    /**
     * The file partition.
     */
    class FilePart : public Part {
    public:
        static FilePart& GetByTag(const TagList::Tag&) noexcept;

        //! Load the super block from the partition.
        FilePart& LoadSuperBlock() noexcept;

        const fs::SuperBlock& GetSuperBlock() const noexcept;

        /**
         * @brief Open the root directory.
         *
         * @note
         * Only one root directory can be open at a time in all partitions.
         */
        void OpenRootDir() const noexcept;

        FileDesc OpenFile(const Path&, bit::Flags<File::OpenMode> flags) noexcept;

        stl::size_t WriteFile(FileDesc, const void* data, stl::size_t size) noexcept;

        stl::size_t ReadFile(FileDesc, void* buf, stl::size_t size) const noexcept;

        stl::size_t SeekFile(FileDesc, stl::int32_t offset, File::SeekOrigin) const noexcept;

        /**
         * @brief Delete a file.
         *
         * @details
         * We should free or synchronize the following items when deleting a file:
         * - The directory entry in its parent directory.
         * - The index node.
         */
        bool DeleteFile(const Path&) noexcept;

        /**
         * @brief Create a directory.
         *
         * @details
         * We should allocate or synchronize the following items when creating a directory:
         * - For its parent directory:
         *   - A new directory entry.
         *   - The index node.
         * - For the new directory:
         *   - A new index node.
         *   - A new block for directory entries.
         *   - Two directory entries `.` and `..`.
         */
        bool CreateDir(const Path&) noexcept;

        fs::Directory* OpenDir(const Path&) const noexcept;

        /**
         * @brief Read the next entry in a directory.
         *
         * @return A directory entry or @p nullptr if there are no more entries.
         */
        fs::DirEntry* ReadDir(const fs::Directory&) const noexcept;

        //! Delete a subdirectory from a directory.
        bool DeleteDir(fs::Directory& parent, const fs::Directory& child) noexcept;

    private:
        enum class BitmapType {
            Node,
            Block,
        };

        /**
         * The track record of a path search.
         */
        struct PathSearchRecord {
            //! The path already found.
            Path searched;

            /**
             * @brief The parent directory of the last found path.
             *
             * @details
             * It has been opened so that we can create entries in it.
             * Users should manually call @p fs::Directory::Close when it is no longer needed.
             */
            fs::Directory* parent {nullptr};

            //! The item type of the last found path.
            fs::FileType type {fs::FileType::Unknown};

            //! The index node ID of the last found item.
            stl::size_t inode_idx {npos};
        };

        //! Open a directory by its index node ID.
        fs::Directory& OpenDir(stl::size_t inode_idx) const noexcept;

        /**
         * @brief Search an entry in a directory.
         *
         * @param name An entry name.
         * @param[out] found_entry The found entry.
         * @return Whether the entry is found.
         */
        bool SearchDirEntry(const fs::Directory&, stl::string_view name,
                            fs::DirEntry& found_entry) const noexcept;

        //! Synchronize an entry in a directory to the partition.
        bool SyncDirEntry(fs::Directory&, const fs::DirEntry&, void* io_buf,
                          stl::size_t io_buf_size) noexcept;

        /**
         * @brief Delete an entry from a directory.
         *
         * @details
         * We should free or synchronize the following items when deleting an entry from a directory:
         * - The directory entry in its parent directory.
         * - The index node.
         */
        bool DeleteDirEntry(fs::Directory&, stl::size_t inode_idx, void* io_buf,
                            stl::size_t io_buf_size) noexcept;

        /**
         * @brief Search a path.
         *
         * @param path A path.
         * @param[out] record The track record.
         * @return Whether the path is found.
         */
        bool SearchPath(const Path& path, PathSearchRecord& record) const noexcept;

        /**
         * @brief Create a file in a directory.
         *
         * @details
         * We should allocate or synchronize the following items when creating a file:
         * - For its parent directory:
         *   - A new directory entry.
         *   - The index node.
         * - For the new file:
         *   - A new index node.
         */
        FileDesc CreateFile(fs::Directory&, stl::string_view name,
                            bit::Flags<File::OpenMode> flags) noexcept;

        stl::size_t ReadFile(const fs::File&, void* buf, stl::size_t size) const noexcept;

        stl::size_t SeekFile(fs::File&, stl::int32_t offset, File::SeekOrigin) const noexcept;

        stl::size_t WriteFile(fs::File&, const void* data, stl::size_t size) noexcept;

        //! Open a file by its index node ID.
        FileDesc OpenFile(stl::size_t inode_idx, bit::Flags<File::OpenMode> flags) const noexcept;

        /**
         * @brief Allocate an index node from the bitmap.
         *
         * @return The index node ID or @p npos if there is no free index node.
         */
        stl::size_t AllocNode() const noexcept;

        /**
         * @brief Allocate a block from the bitmap.
         *
         * @return The block LBA or @p npos if there is no free block.
         */
        stl::size_t AllocBlock() const noexcept;

        //! Free an index node to the bitmap.
        const FilePart& FreeNode(stl::size_t idx) const noexcept;

        //! Free a block to the bitmap.
        const FilePart& FreeBlock(stl::size_t lba) const noexcept;

        /**
         * @brief Synchronize an index node's bit from the in-memory bitmap to the partition.
         *
         * @param idx An index node ID.
         */
        FilePart& SyncNodeBitmap(stl::size_t idx) noexcept;

        /**
         * @brief Synchronize a block's bit from the in-memory bitmap to the partition.
         *
         * @param idx A block LBA.
         */
        FilePart& SyncBlockBitmap(stl::size_t lba) noexcept;

        //! Load an index node from the partition.
        fs::IdxNode& OpenNode(stl::size_t idx) const noexcept;

        /**
         * @brief Delete an index node from the partition.
         *
         * @details
         * We should free or synchronize the following items when deleting an index node:
         * - The blocks.
         * - The single indirect block table.
         * - The index node.
         */
        FilePart& DeleteNode(stl::size_t idx) noexcept;

        //! Zero-fill an index node in the partition.
        FilePart& ZeroFillNode(stl::size_t idx, void* io_buf, stl::size_t io_buf_size) noexcept;

        //! Synchronize an index node to the partition.
        FilePart& SyncNode(const fs::IdxNode&, void* io_buf, stl::size_t io_buf_size) noexcept;

        //! Load the block bitmap from the partition.
        const FilePart& LoadBlockBitmap() const noexcept;

        //! Load the index node bitmap from the partition.
        const FilePart& LoadNodeBitmap() const noexcept;

        //! Synchronize a bit in an in-memory bitmap to the partition.
        FilePart& SyncBitmap(BitmapType, stl::size_t bit_idx) noexcept;

        fs::SuperBlock* super_block_ {nullptr};

        /**
         * @brief The block bitmap.
         *
         * @details
         * In our system, a block is a sector. It is the smallest allocation unit.
         * They are used to save file and directory data.
         */
        mutable Bitmap block_bitmap_;

        /**
         * @brief The index node bitmap.
         *
         * @details
         * The total number of index nodes in a partition is fixed during file system initialization.
         * When there is no free index node, we cannot create more files or directories.
         */
        mutable Bitmap inode_bitmap_;

        //! The list of open index nodes.
        mutable TagList open_inodes_;
    };

    /**
     * Disk information.
     */
    class Info {
    public:
        //! Initialize disk information based on the data returned by identify commands.
        explicit Info(const stl::byte*) noexcept;

        stl::string_view GetSerial() const noexcept;

        stl::string_view GetModel() const noexcept;

        stl::size_t GetSectorCount() const noexcept;

    private:
        static constexpr stl::size_t serial_len {20};
        static constexpr stl::size_t model_len {40};

        stl::array<char, serial_len + 1> serial_;
        stl::array<char, model_len + 1> model_;
        stl::size_t sector_count_ {0};
    };

    using PrimaryParts = stl::array<FilePart, prim_part_count>;
    using LogicParts = stl::array<FilePart, max_logic_part_count>;

    /**
     * Disk commands.
     */
    enum class Cmd {
        //! Read data.
        Read = 0x20,
        //! Write data.
        Write = 0x30,
        //! Identify device data.
        Identify = 0xEC
    };

    Disk() noexcept = default;

    Disk(const Disk&) = delete;

    /**
     * @brief Read words from the disk.
     *
     * @note
     * The disk can only read or write a word once, cannot operate bytes.
     */
    const Disk& ReadSectors(stl::size_t start_lba, void* buf, stl::size_t count = 1) const noexcept;

    /**
     * @brief Write words to the disk.
     *
     * @note
     * The disk can only read or write a word once, cannot operate bytes.
     */
    Disk& WriteSectors(stl::size_t start_lba, const void* data, stl::size_t count = 1) noexcept;

    //! Attach the disk to an IDE channel.
    Disk& Attach(IdeChnl* ide_chnl, stl::size_t idx) noexcept;

    //! Scan and initialize partitions.
    Disk& ScanParts() noexcept;

    //! Get disk information.
    Info GetInfo() const noexcept;

    const PrimaryParts& GetPrimaryParts() const noexcept;

    const LogicParts& GetLogicParts() const noexcept;

    const FilePart& GetPrimaryPart(stl::size_t) const noexcept;

    const FilePart& GetLogicPart(stl::size_t) const noexcept;

    PrimaryParts& GetPrimaryParts() noexcept;

    LogicParts& GetLogicParts() noexcept;

    FilePart& GetPrimaryPart(stl::size_t) noexcept;

    FilePart& GetLogicPart(stl::size_t) noexcept;

    stl::string_view GetName() const noexcept;

    Disk& SetName(stl::string_view) noexcept;

private:
    static constexpr stl::size_t name_len {8};

    //! Select the disk as the command target.
    const Disk& Select() const noexcept;

    //! Set a sector range for reading or writing commands.
    const Disk& SetSectors(stl::size_t start_lba, stl::size_t count = 1) const noexcept;

    //! Send a command to the target disk.
    const Disk& SendCmd(Cmd) const noexcept;

    const IdeChnl& GetIdeChnl() const noexcept;

    IdeChnl& GetIdeChnl() noexcept;

    /**
     * @brief Wait for a while when the disk is busy.
     *
     * @return Whether the data is prepared.
     */
    bool BusyWait() const noexcept;

    const Disk& ReadWords(void* buf, stl::size_t count = 1) const noexcept;

    Disk& WriteWords(const void* data, stl::size_t count = 1) noexcept;

    /**
     * @brief Scan and initialize partitions.
     *
     * @details
     * At first, disks were small in capacity and did not need many partitions. One partition table was enough.
     * But as the capacity grew, four partitions were no longer enough.
     * To ensure compatibility, one of the partitions is used as an extended partition.
     * It contains several logical partitions, each of which is logically equivalent to a separate disk with a partition table.
     * The partition table is in the beginning sector of each logical disk,
     * which has the same structure as the master boot record, called extended boot record.
     * A partition table in an extended boot record also has four entries:
     * 1. The information of the current logical partition.
     * 2. The LBA of the next current logical partition.
     * 3. N/A.
     * 4. N/A.
     * ```
     *                              Disk
     *                      ┌───────────────────┐
     *                      │        MBR        │
     *                      │┌─────────────────┐│
     *                      ││ Partition Table ││
     *                      │└─────────────────┘│
     *                    ▲ ├───────────────────┤
     *                    │ │    Boot Sector    │
     *  Primary Partition │ ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤
     *                    │ │       Data        │
     *                    ─ ├───────────────────┤
     *                    │ │    Boot Sector    │
     *  Primary Partition │ ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤
     *                    │ │       Data        │
     *                    ─ ├───────────────────┤ ▲
     *                    │ │        EBR        │ │
     *                    │ │┌─────────────────┐│ │
     *                    │ ││ Partition Table ││ │
     *                    │ │└─────────────────┘│ │ Logical Disk
     *                    │ ├───────────────────┤ │ ▲
     *                    │ │    Boot Sector    │ │ │
     *                    │ ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤ │ │ Logical Partition
     *                    │ │       Data        │ │ │
     * Extended Partition │ ├───────────────────┤ ─ ▼
     *                    │ │        EBR        │ │
     *                    │ │┌─────────────────┐│ │
     *                    │ ││ Partition Table ││ │
     *                    │ │└─────────────────┘│ │ Logical Disk
     *                    │ ├───────────────────┤ │ ▲
     *                    │ │    Boot Sector    │ │ │
     *                    │ ├ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┤ │ │ Logical Partition
     *                    │ │       Data        │ │ │
     *                    ▼ └───────────────────┘ ▼ ▼
     * ```
     *
     * @param lba The LBA of a boot record.
     * @param new_disk Whether a new disk is being scanned.
     */
    Disk& ScanParts(stl::size_t lba, bool new_disk = false) noexcept;

    stl::array<char, name_len + 1> name_;

    //! Primary partitions.
    PrimaryParts prim_parts_;

    //! Logical partitions.
    LogicParts logic_parts_;

    //! The IDE channel.
    IdeChnl* ide_chnl_ {nullptr};

    //! The index of the disk under the IDE channel.
    stl::size_t idx_ {0};
};

//! The index of the boot disk.
inline constexpr stl::size_t boot_disk_idx {0};
//! The number of boot sectors on the boot disk.
inline constexpr stl::size_t boot_sector_count {1};

//! Get the number of disks.
stl::size_t GetDiskCount() noexcept;

//! Get disk partitions.
TagList& GetDiskParts() noexcept;

//! Initialize disks.
void InitDisk() noexcept;

//! Whether disks have been initialized.
bool IsDiskInited() noexcept;

//! Initialize the file system.
void InitFileSys() noexcept;

//! Get the default partition.
Disk::FilePart& GetDefaultPart() noexcept;

namespace fs {

//! Get the root directory.
Directory& GetRootDir() noexcept;

}  // namespace fs

}  // namespace io