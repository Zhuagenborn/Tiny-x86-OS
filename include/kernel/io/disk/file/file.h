/**
 * @file file.h
 * @brief Underlying file storage.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/io/file/file.h"
#include "kernel/io/video/print.h"
#include "kernel/stl/array.h"
#include "kernel/util/bit.h"

namespace io::fs {

//! The maximum number of files that can be open simultaneously in the system.
inline constexpr stl::size_t max_open_file_times {32};

class IdxNode;

struct File {
    File() noexcept = default;

    File(File&&) noexcept;

    bool IsOpen() const noexcept;

    IdxNode& GetNode() const noexcept;

    stl::size_t GetNodeIdx() const noexcept;

    File& Clear() noexcept;

    void Close() noexcept;

    //! Open and access modes.
    bit::Flags<io::File::OpenMode> flags {0};

    /**
     * The index node for file content storage.
     * It is @p nullptr when the file is not open.
     */
    IdxNode* inode {nullptr};

    //! The access offset.
    mutable stl::size_t pos {0};
};

/**
 * @brief The open file table.
 *
 * @details
 * The open file table saves all open files in the system.
 * A global file descriptor is an index to this table.
 */
template <stl::size_t size>
class FileTab {
    static_assert(size > std_stream_count);

public:
    /**
     * @brief Get a free descriptor.
     *
     * @return A free descriptor or @p npos if there is no free descriptor.
     */
    FileDesc GetFreeDesc() const noexcept {
        for (stl::size_t i {std_stream_count}; i != files_.size(); ++i) {
            if (!files_[i].IsOpen()) {
                return i;
            }
        }

        io::PrintlnStr("The system file table is full.");
        return npos;
    }

    //! Whether an index node is open.
    bool Contain(const stl::size_t inode_idx) const noexcept {
        for (stl::size_t i {std_stream_count}; i != files_.size(); ++i) {
            if (files_[i].IsOpen() && files_[i].GetNodeIdx() == inode_idx) {
                return true;
            }
        }

        return false;
    }

    //! Get the file by a descriptor.
    const File& operator[](const FileDesc desc) const noexcept {
        dbg::Assert(desc < files_.size());
        return files_[desc];
    }

    File& operator[](const FileDesc desc) noexcept {
        return const_cast<File&>(const_cast<const FileTab&>(*this)[desc]);
    }

    constexpr stl::size_t GetSize() const noexcept {
        return files_.size();
    }

private:
    //! Open files.
    stl::array<File, size> files_;
};

//! Get the open file table.
FileTab<max_open_file_times>& GetFileTab() noexcept;

}  // namespace io::fs