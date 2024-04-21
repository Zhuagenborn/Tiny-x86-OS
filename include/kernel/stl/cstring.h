#pragma once

#include "kernel/stl/cerron.h"
#include "kernel/stl/cstdint.h"

namespace stl {

constexpr size_t strlen(const char* const str) noexcept {
    size_t len {0};
    if (str) {
        while (str[len] != '\0') {
            ++len;
        }
    }
    return len;
}

char* strcpy(char* dest, const char* src) noexcept;

errno_t strcpy_s(char* dest, size_t dest_size, const char* src) noexcept;

int strcmp(const char* lhs, const char* rhs) noexcept;

char* strcat(char* dest, const char* src) noexcept;

constexpr char* strrchr(const char* str, const char ch) noexcept {
    if (!str) {
        return nullptr;
    }

    const char* last {nullptr};
    while (*str != '\0') {
        if (*str == ch) {
            last = str;
        }
        ++str;
    }

    return const_cast<char*>(last);
}

constexpr char* strchr(const char* str, const char ch) noexcept {
    if (!str) {
        return nullptr;
    }

    while (*str != '\0') {
        if (*str == ch) {
            return const_cast<char*>(str);
        } else {
            ++str;
        }
    }

    return nullptr;
}

void memset(void* addr, byte val, size_t size) noexcept;

void memcpy(void* dest, const void* src, size_t size) noexcept;

int memcmp(const void* lhs, const void* rhs, size_t size) noexcept;

}  // namespace stl