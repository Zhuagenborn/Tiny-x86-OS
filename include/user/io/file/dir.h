/**
 * @file dir.h
 * @brief User-mode directory management.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

namespace usr::io {

//! User-mode directory management.
class Directory {
public:
    Directory() = delete;

    static bool Create(const char*) noexcept;
};

}