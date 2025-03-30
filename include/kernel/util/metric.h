/**
 * @file metric.h
 * @brief Metrics.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/stl/cstdint.h"

//! The value of an invalid index.
inline constexpr stl::size_t npos {static_cast<stl::size_t>(-1)};

//! Convert kilobytes to bytes.
constexpr stl::size_t KB(const stl::size_t size) noexcept {
    return size * 1024;
}

//! Convert megabytes to bytes.
constexpr stl::size_t MB(const stl::size_t size) noexcept {
    return size * KB(1024);
}

//! Convert gigabytes to bytes.
constexpr stl::size_t GB(const stl::size_t size) noexcept {
    return size * MB(2014);
}

//! Convert seconds to milliseconds.
constexpr stl::size_t SecondsToMilliseconds(const stl::size_t seconds) noexcept {
    return seconds * 1000;
}

template <typename T>
constexpr T RoundUpDivide(const T dividend, const T divisor) noexcept {
    return (dividend + divisor - 1) / divisor;
}

//! Align a value backward.
template <typename T>
constexpr T BackwardAlign(const T val, const T align) noexcept {
    return val - val % align;
}

//! Align a value forward.
template <typename T>
constexpr T ForwardAlign(const T val, const T align) noexcept {
    return RoundUpDivide(val, align) * align;
}