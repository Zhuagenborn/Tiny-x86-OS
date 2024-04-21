#pragma once

#include "kernel/stl/array.h"

namespace stl {

template <typename T>
class span {
public:
    using value_type = T;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using reference = value_type&;
    using const_reference = const value_type&;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using iterator = value_type*;
    using const_iterator = const value_type*;
    using reverse_iterator = stl::reverse_iterator<iterator>;
    using const_reverse_iterator = stl::reverse_iterator<const_iterator>;

    constexpr span() noexcept = default;

    constexpr span(const const_pointer vals, const size_type size) noexcept :
        vals_ {vals}, size_ {size} {}

    template <size_type n>
    constexpr span(const array<T, n>& vals) noexcept : vals_ {vals.data()}, size_ {vals.size()} {}

    constexpr bool empty() const noexcept {
        return size_ == 0;
    }

    constexpr size_type size() const noexcept {
        return size_;
    }

    constexpr const_reference operator[](const size_type idx) const noexcept {
        return vals_[idx];
    }

    constexpr const_pointer data() const noexcept {
        return vals_;
    }

    constexpr const_reference front() const {
        return vals_[0];
    }

    constexpr const_reference back() const {
        return vals_[size_ - 1];
    }

    constexpr const_iterator begin() const noexcept {
        return vals_;
    }

    constexpr const_iterator end() const noexcept {
        return vals_ + size_;
    }

    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    constexpr const_iterator cend() const noexcept {
        return end();
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator {end()};
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator {begin()};
    }

    constexpr const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    constexpr const_reverse_iterator crend() const noexcept {
        return rend();
    }

private:
    const_pointer vals_ {nullptr};
    size_type size_ {0};
};

}  // namespace stl