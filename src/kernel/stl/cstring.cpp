#include "kernel/stl/cstring.h"
#include "kernel/debug/assert.h"

namespace stl {

char* strcpy(char* dest, const char* src) noexcept {
    dbg::Assert(dest && src);
    const auto begin {dest};
    while ((*dest++ = *src++) != '\0') {
    }
    return begin;
}

errno_t strcpy_s(char* dest, const size_t dest_size, const char* src) noexcept {
    dbg::Assert(src);
    dbg::Assert(dest && dest_size > 0);
    const auto begin {dest};
    while (static_cast<size_t>(dest - begin) != dest_size && (*dest++ = *src++)) {
    }

    *(dest - 1) = '\0';
    return 0;
}

int strcmp(const char* lhs, const char* rhs) noexcept {
    dbg::Assert(lhs && rhs);
    while (*lhs != '\0' && *lhs == *rhs) {
        ++lhs;
        ++rhs;
    }

    if (*lhs != *rhs) {
        return *lhs > *rhs ? 1 : -1;
    } else {
        return 0;
    }
}

void memset(void* const addr, const byte val, const size_t size) noexcept {
    dbg::Assert(addr);
    for (size_t i {0}; i != size; ++i) {
        *(static_cast<byte*>(addr) + i) = val;
    }
}

void memcpy(void* const dest, const void* const src, const size_t size) noexcept {
    dbg::Assert(dest && src);
    for (size_t i {0}; i != size; ++i) {
        *(static_cast<byte*>(dest) + i) = *(static_cast<const byte*>(src) + i);
    }
}

int memcmp(const void* const lhs, const void* const rhs, const size_t size) noexcept {
    dbg::Assert(lhs && rhs);
    for (size_t i {0}; i != size; ++i) {
        const auto v1 {*(static_cast<const byte*>(lhs) + i)};
        const auto v2 {*(static_cast<const byte*>(rhs) + i)};
        if (v1 != v2) {
            return v1 > v2 ? 1 : -1;
        }
    }

    return 0;
}

char* strcat(char* const dest, const char* src) noexcept {
    dbg::Assert(dest && src);
    char* str {dest};
    while (*str != '\0') {
        ++str;
    }

    while ((*str++ = *src++) != '\0') {
    }

    return dest;
}

}  // namespace stl