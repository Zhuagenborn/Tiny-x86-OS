#pragma once

#include "kernel/stl/cstdint.h"
#include "kernel/stl/iterator.h"

namespace stl {

template <typename T, size_t n>
class array {
    static_assert(n > 0);

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

    constexpr array() noexcept = default;

    constexpr array(const value_type (&vals)[n]) noexcept : vals_ {vals} {}

    constexpr bool empty() const noexcept {
        return n == 0;
    }

    constexpr size_type size() const noexcept {
        return n;
    }

    constexpr size_type max_size() const noexcept {
        return n;
    }

    constexpr const_reference operator[](const size_type idx) const noexcept {
        return vals_[idx];
    }

    constexpr reference operator[](const size_type idx) noexcept {
        return const_cast<reference>(const_cast<const array&>(*this)[idx]);
    }

    constexpr pointer data() noexcept {
        return const_cast<T*>(const_cast<const array&>(*this).data());
    }

    constexpr const_pointer data() const noexcept {
        return vals_;
    }

    constexpr reference front() {
        return const_cast<reference>(const_cast<const array&>(*this).front());
    }

    constexpr const_reference front() const {
        return vals_[0];
    }

    constexpr reference back() {
        return const_cast<reference>(const_cast<const array&>(*this).back());
    }

    constexpr const_reference back() const {
        return vals_[n - 1];
    }

    constexpr iterator begin() noexcept {
        return const_cast<iterator>(const_cast<const array&>(*this).begin());
    }

    constexpr const_iterator begin() const noexcept {
        return vals_;
    }

    constexpr iterator end() noexcept {
        return const_cast<iterator>(const_cast<const array&>(*this).end());
    }

    constexpr const_iterator end() const noexcept {
        return vals_ + n;
    }

    constexpr const_iterator cbegin() const noexcept {
        return begin();
    }

    constexpr const_iterator cend() const noexcept {
        return end();
    }

    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator {end()};
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator {end()};
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator {begin()};
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
    value_type vals_[n] {};
};

}  // namespace stl