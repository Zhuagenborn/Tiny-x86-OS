/**
 * @file inode.h
 * @brief The index node.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/array.h"
#include "kernel/util/metric.h"
#include "kernel/util/tag_list.h"

namespace io::fs {

/**
 * @brief The index node.
 *
 * @details
 * In @em Linux, the index node describes a file system object such as a file or a directory.
 * Each index node stores the attributes and disk block locations of the object's data.
 *
 * In our system, an index node has 12 direct blocks and a single indirect block table.
 * The size of a single indirect block table is one sector, so it can save 128 block addresses.
 * Totally, an index node has up to 140 blocks for data storage.
 *
 * @code
 *                      Index Node
 * ┌────────────┬───────────────┬───────────────────────┐
 * │ Attributes │ Direct Blocks │ Single Indirect Block │
 * └────────────┴───────────────┴───────────────────────┘
 *                │          │              │
 *                ▼          ▼              │
 *            ┌───────┐  ┌───────┐          ▼
 *            │ Block │  │ Block │  ┌───────────────┐
 *            └───────┘  └───────┘  │ Direct Blocks │
 *                                  └───────────────┘
 *                                      │        │
 *                                      ▼        ▼
 *                                  ┌───────┐ ┌───────┐
 *                                  │ Block │ │ Block │
 *                                  └───────┘ └───────┘
 * @endcode
 *
 * Index nodes do not indicate their data type.
 * Instead, we use directory entries @p DirEntry to determine whether an item is a file or a directory.
 */
struct IdxNode {
    static constexpr stl::size_t direct_block_count {12};

    static IdxNode& GetByTag(const TagList::Tag&) noexcept;

    IdxNode() noexcept;

    IdxNode(const IdxNode&) = delete;

    IdxNode& Init() noexcept;

    /**
     * @brief Close the index node.
     *
     * @details
     * If it is not used by any task, it will be removed from the list of open index nodes,
     * and its memory will be rreed.
     */
    void Close() noexcept;

    bool IsOpen() const noexcept;

    stl::size_t GetIndirectTabLba() const noexcept;

    stl::size_t GetDirectLba(stl::size_t idx) const noexcept;

    IdxNode& SetIndirectTabLba(stl::size_t) noexcept;

    IdxNode& SetDirectLba(stl::size_t idx, stl::size_t lba) noexcept;

    //! Clone a new index node but reset its open times, writing status and tag.
    void CloneToPure(IdxNode&) const noexcept;

    //! The tag for the list of open index nodes.
    TagList::Tag tag;

    //! The ID or index.
    stl::size_t idx {npos};

    /**
     * @brief The data size.
     *
     * @details
     * - If the index node refers to a file, it is the size of the file.
     * - If the index node refers to a directory, it is the total size of all entries in the directory.
     */
    stl::size_t size {0};

    //! The number of times the file has been opened.
    stl::size_t open_times {0};

    //! Whether the file is being written.
    bool write_deny {false};

private:
    //! The LBAs of direct blocks.
    stl::array<stl::size_t, direct_block_count> direct_lbas_;

    /**
     * @brief The LBA of the single indirect block table.
     *
     * @details
     * Each entry in the single indirect block table is a block's LBA.
     */
    stl::size_t indirect_tab_lba_;
};

//! The index of the root directory's index node.
inline constexpr stl::size_t root_inode_idx {0};

}  // namespace io::fs