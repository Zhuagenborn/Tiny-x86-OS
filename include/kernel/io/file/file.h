/**
 * @file file.h
 * @brief File management.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/io/file/path.h"
#include "kernel/util/bit.h"
#include "kernel/util/metric.h"

namespace io {

//! The file descriptor.
class FileDesc {
public:
    constexpr FileDesc(const stl::size_t desc = npos) noexcept : desc_ {desc} {}

    constexpr bool IsValid() const noexcept {
        return desc_ != npos;
    }

    constexpr operator stl::size_t() const noexcept {
        return desc_;
    }

    constexpr FileDesc& Reset() noexcept {
        desc_ = npos;
        return *this;
    }

    void Close() noexcept;

private:
    stl::size_t desc_;
};

//! The wrapper for file functions of @p Disk::FilePart.
class File {
public:
    enum class OpenMode { ReadOnly = 0, WriteOnly = 1, ReadWrite = 2, CreateNew = 4 };

    enum class SeekOrigin { Begin, Curr, End };

    static FileDesc Open(const Path&, bit::Flags<OpenMode>) noexcept;

    static bool Delete(const Path&) noexcept;

    explicit File(FileDesc) noexcept;

    explicit File(const Path&, bit::Flags<OpenMode>) noexcept;

    File(File&&) noexcept;

    ~File() noexcept;

    stl::size_t Write(const void* data, stl::size_t size) noexcept;

    stl::size_t Read(void* buf, stl::size_t size) noexcept;

    stl::size_t Seek(stl::int32_t offset, SeekOrigin) noexcept;

    void Close() noexcept;

    bool IsOpen() const noexcept;

private:
    FileDesc desc_;
};

//! The standard input stream.
inline constexpr FileDesc std_in {0};
//! The standard output stream.
inline constexpr FileDesc std_out {1};
//! The standard error stream.
inline constexpr FileDesc std_err {2};

//! The number of standard streams.
inline constexpr stl::size_t std_stream_count {3};

//! System calls.
namespace sc {

class File {
public:
    File() = delete;

    struct OpenArgs {
        const char* path;
        stl::uint32_t flags;
    };

    struct WriteArgs {
        stl::size_t desc;
        const void* data;
        stl::size_t size;
    };

    struct ReadArgs {
        stl::size_t desc;
        void* buf;
        stl::size_t size;
    };

    struct SeekArgs {
        stl::size_t desc;
        stl::int32_t offset;
        io::File::SeekOrigin origin;
    };

    static stl::size_t Open(const OpenArgs&) noexcept;

    static void Close(stl::size_t) noexcept;

    static bool Delete(const char*) noexcept;

    static stl::size_t Write(const WriteArgs&) noexcept;

    static stl::size_t Read(const ReadArgs&) noexcept;

    static stl::size_t Seek(const SeekArgs&) noexcept;
};

}  // namespace sc

}  // namespace io