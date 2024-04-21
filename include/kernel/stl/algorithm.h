#pragma once

namespace stl {

template <typename InputIter, typename T>
InputIter find(const InputIter begin, const InputIter end, const T& val) noexcept {
    for (auto it {begin}; it != end; ++it) {
        if (*it == val) {
            return it;
        }
    }

    return end;
}

template <typename T>
constexpr const T& min(const T& lhs, const T& rhs) noexcept {
    return lhs < rhs ? lhs : rhs;
}

template <typename T>
constexpr const T& max(const T& lhs, const T& rhs) noexcept {
    return lhs > rhs ? lhs : rhs;
}

}  // namespace stl