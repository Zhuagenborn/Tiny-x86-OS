/**
 * Directory management.
 */

#pragma once

#include "kernel/io/file/path.h"

namespace io {

/**
 * The wrapper for directory functions of @p Disk::FilePart.
 */
class Directory {
public:
    //! Create a directory.
    static bool Create(const Path&) noexcept;
};

//! System calls.
namespace sc {

class Directory {
public:
    Directory() = delete;

    static bool Create(const char*) noexcept;
};

}  // namespace sc

}  // namespace io