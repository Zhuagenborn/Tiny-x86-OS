#pragma once

#include "kernel/stl/cstdint.h"

namespace stl {

class input_iterator_tag {};
class output_iterator_tag {};
class forward_iterator_tag : public input_iterator_tag {};
class bidirectional_iterator_tag : public forward_iterator_tag {};
class random_access_iterator_tag : public bidirectional_iterator_tag {};

template <typename Category, typename T, typename Distance = ptrdiff_t, typename Pointer = T*,
          typename Reference = T&>
struct iterator {
    using iterator_category = Category;
    using value_type = T;
    using pointer = Pointer;
    using reference = Reference;
    using difference_type = Distance;
};

template <typename Iterator>
struct iterator_traits {
    using iterator_category = typename Iterator::iterator_category;
    using value_type = typename Iterator::value_type;
    using pointer = typename Iterator::pointer;
    using reference = typename Iterator::reference;
    using difference_type = typename Iterator::difference_type;
};

template <typename T>
struct iterator_traits<T*> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = T*;
    using reference = T&;
    using difference_type = ptrdiff_t;
};

template <typename T>
struct iterator_traits<const T*> {
    using iterator_category = random_access_iterator_tag;
    using value_type = T;
    using pointer = const T*;
    using reference = const T&;
    using difference_type = ptrdiff_t;
};

template <typename Iterator>
class reverse_iterator {
public:
    using iterator_category = typename iterator_traits<Iterator>::iterator_category;
    using value_type = typename iterator_traits<Iterator>::value_type;
    using pointer = typename iterator_traits<Iterator>::pointer;
    using reference = typename iterator_traits<Iterator>::reference;
    using difference_type = typename iterator_traits<Iterator>::difference_type;

    constexpr reverse_iterator() noexcept = default;

    constexpr reverse_iterator(const reverse_iterator<Iterator>& it) noexcept : curr_ {it.curr_} {}

    constexpr explicit reverse_iterator(const Iterator it) noexcept : curr_ {it} {}

    constexpr Iterator base() const noexcept {
        return curr_;
    }

    constexpr reference operator*() const noexcept {
        auto tmp {curr_};
        return *--tmp;
    }

    constexpr pointer operator->() const noexcept {
        return &operator*();
    }

    constexpr reverse_iterator<Iterator>& operator++() noexcept {
        --curr_;
        return *this;
    }

    constexpr reverse_iterator<Iterator>& operator++(int) noexcept {
        const auto old {*this};
        --curr_;
        return old;
    }

    constexpr reverse_iterator<Iterator>& operator--() noexcept {
        ++curr_;
        return *this;
    }

    constexpr reverse_iterator<Iterator>& operator--(int) noexcept {
        const auto old {*this};
        ++curr_;
        return old;
    }

    constexpr reverse_iterator<Iterator> operator+(const difference_type n) const noexcept {
        return curr_ - n;
    }

    constexpr reference operator[](const difference_type n) const noexcept {
        return *(*this + n);
    }

private:
    Iterator curr_ {};
};

}  // namespace stl