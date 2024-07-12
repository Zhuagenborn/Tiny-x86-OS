/**
 * User-mode directory management.
 */

#pragma once

namespace usr::io {

class Directory {
public:
    Directory() = delete;

    static bool Create(const char*) noexcept;
};

}