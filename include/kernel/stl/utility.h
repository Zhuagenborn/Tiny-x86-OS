#pragma once

#include "kernel/stl/type_traits.h"

namespace stl {

template <typename T>
constexpr remove_reference_t<T>&& move(T&& t) noexcept {
    return static_cast<remove_reference_t<T>&&>(t);
}

template <typename T>
constexpr void swap(T& lhs, T& rhs) noexcept {
    T tmp {move(lhs)};
    lhs = move(rhs);
    rhs = move(tmp);
}

}  // namespace stl