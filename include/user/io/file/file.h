/**
 * User-mode file management.
 */

#pragma once

#include "user/stl/cstdint.h"

namespace usr::io {

class File {
public:
    File() = delete;

    enum class OpenMode { ReadOnly = 0, WriteOnly = 1, ReadWrite = 2, CreateNew = 4 };

    enum class SeekOrigin { Begin, Curr, End };

    static stl::size_t Open(const char*, stl::uint32_t) noexcept;

    static void Close(stl::size_t desc) noexcept;

    static bool Delete(const char* path) noexcept;

    static stl::size_t Write(stl::size_t desc, const void* data, stl::size_t size) noexcept;

    static stl::size_t Read(stl::size_t desc, void* buf, stl::size_t size) noexcept;

    static stl::size_t Seek(stl::size_t desc, stl::int32_t offset, SeekOrigin) noexcept;
};

}  // namespace usr::io