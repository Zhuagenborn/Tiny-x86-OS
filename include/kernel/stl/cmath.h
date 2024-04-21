#pragma once

namespace stl {

template <typename T>
constexpr T abs(const T num) noexcept {
    return num >= 0 ? num : -num;
}

}  // namespace stl