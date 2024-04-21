#pragma once

#include "kernel/stl/algorithm.h"
#include "kernel/stl/cstring.h"

namespace stl {

class string_view {
public:
    using value_type = char;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using const_iterator = const value_type*;
    using iterator = const_iterator;

    static constexpr size_type npos {static_cast<size_type>(-1)};

    constexpr string_view(const const_pointer str = nullptr) noexcept : str_ {str} {
        if (str_) {
            len_ = strlen(str_);
        }
    }

    constexpr string_view(const const_pointer str, const size_type len) noexcept :
        str_ {str}, len_ {len} {}

    constexpr const_pointer data() const noexcept {
        return str_;
    }

    constexpr bool empty() const noexcept {
        return len_ == 0;
    }

    constexpr size_type size() const noexcept {
        return len_;
    }

    constexpr size_type rfind(const value_type ch) const noexcept {
        if (const auto sub_str {strrchr(str_, ch)}; sub_str) {
            return sub_str - str_;
        } else {
            return npos;
        }
    }

    constexpr size_type find(const value_type ch) const noexcept {
        if (const auto sub_str {strchr(str_, ch)}; sub_str) {
            return sub_str - str_;
        } else {
            return npos;
        }
    }

    constexpr const_reference operator[](const size_type idx) const noexcept {
        return str_[idx];
    }

    constexpr const_iterator begin() const noexcept {
        return str_;
    }

    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    constexpr const_iterator end() const noexcept {
        return str_ + len_;
    }

    constexpr const_iterator cend() const noexcept {
        return end();
    }

    constexpr reference front() {
        return const_cast<reference>(const_cast<const string_view&>(*this).front());
    }

    constexpr const_reference front() const {
        return str_[0];
    }

    constexpr reference back() {
        return const_cast<reference>(const_cast<const string_view&>(*this).back());
    }

    constexpr const_reference back() const {
        return str_[len_ - 1];
    }

    string_view substr(const size_type idx = 0, const size_type count = npos) const noexcept {
        if (idx >= len_) {
            return nullptr;
        }

        if (const auto actual_count {min(count, len_ - idx)}; actual_count > 0) {
            return {str_ + idx, actual_count};
        } else {
            return nullptr;
        }
    }

private:
    const_pointer str_;
    size_type len_ {0};
};

constexpr bool operator==(const string_view lhs, const string_view rhs) noexcept {
    if (lhs.empty() && rhs.empty()) {
        return true;
    } else if (!lhs.empty() && !rhs.empty()) {
        return strcmp(lhs.data(), rhs.data()) == 0;
    } else {
        return false;
    }
}

constexpr bool operator!=(const string_view lhs, const string_view rhs) noexcept {
    return !(lhs == rhs);
}

}  // namespace stl