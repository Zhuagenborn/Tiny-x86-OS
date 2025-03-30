/**
 * @file dir.h
 * @brief Underlying directory storage.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/io/file/file.h"
#include "kernel/stl/string_view.h"

namespace io::fs {

class IdxNode;

struct Directory {
    /**
     * @brief The minimum number of entries in a directory.
     *
     * @details
     * A directory at least has two entries:
     * - The current directory.
     * - The parent directory.
     */
    static constexpr stl::size_t min_entry_count {2};

    Directory() noexcept = default;

    Directory(const Directory&) = delete;

    void Close() noexcept;

    bool IsOpen() const noexcept;

    bool IsEmpty() const noexcept;

    IdxNode& GetNode() const noexcept;

    stl::size_t GetNodeIdx() const noexcept;

    //! Reset the access offset to the begging of the directory.
    void Rewind() noexcept;

    /**
     * @brief The index node for directory entry storage.
     *
     * @details
     * It is @p nullptr when the directory is not open.
     */
    IdxNode* inode {nullptr};

    //! The access offset.
    mutable stl::size_t pos {0};
};

enum class FileType { Unknown, Regular, Directory };

/**
 * @brief The directory entry.
 *
 * @details
 * The directory entry represents an item in a directory.
 * It can be a file or a directory.
 *
 * Index nodes @p IdxNode do not indicate their data type.
 * Instead, we use directory entries to determine whether an item is a file or a directory.
 *
 * @code
 *                                       Root Directory
 *                          ┌──────────────────┬──────────────────┐
 *                          │                  │   Name: "file"   │
 *                          │                  ├──────────────────┤
 *                          │ Directory Entry  │ Index Node ID: 3 │ ──────┐
 * ┌────────────────┐       │                  ├──────────────────┤       │
 * │ Root Directory │       │                  │    Type: File    │       │
 * │   Index Node   │ ────► ├──────────────────┼──────────────────┤       │
 * └────────────────┘       │                  │   Name: "dir"    │       │
 *                          │                  ├──────────────────┤       │
 *                          │ Directory Entry  │ Index Node ID: 2 │ ───┐  │
 *                          │                  ├──────────────────┤    │  │
 *                          │                  │ Type: Directory  │    │  │
 *                          ├──────────────────┴──────────────────┤    │  │
 *                          │                 ...                 │    │  │
 *                          └─────────────────────────────────────┘    │  │
 *                                                                     │  │
 *          ┌──────────────────────────────────────────────────────────┘  │
 *          │                                                             │
 *          │                         "dir" Directory                     │
 *          │              ┌──────────────────┬───────────────┐           │
 *          │              │                  │     Name      │           │
 *          ▼              │                  ├───────────────┤           │
 * ┌────────────────┐      │ Directory Entry  │ Index Node ID │           │
 * │ Index Node (2) │ ───► │                  ├───────────────┤           │
 * └────────────────┘      │                  │     Type      │           │
 *                         ├──────────────────┴───────────────┤           │
 *                         │               ...                │           │
 *                         └──────────────────────────────────┘           │
 *                                                                        │
 *          ┌─────────────────────────────────────────────────────────────┘
 *          ▼
 * ┌────────────────┐     ┌─────────────┐
 * │ Index Node (3) │ ──► │ "file" Data │
 * └────────────────┘     └─────────────┘
 * @endcode
 */
struct DirEntry {
    DirEntry() noexcept = default;

    DirEntry(FileType, stl::string_view name, stl::size_t inode_idx) noexcept;

    DirEntry& SetName(stl::string_view) noexcept;

    //! The entry type.
    FileType type {FileType::Unknown};

    //! The directory or file name.
    stl::array<char, Path::max_name_len + 1> name;

    //! The ID of the index node.
    stl::size_t inode_idx {npos};
};

}  // namespace io::fs