/**
 * The bitmap.
 */

#pragma once

#include "kernel/stl/cstdint.h"

class Bitmap {
public:
    Bitmap() noexcept = default;

    /**
     * @brief Create a bitmap.
     *
     * @param bits A bit buffer.
     * @param byte_len The length of the buffer in bytes.
     * @param clear Whether to clear all bits in the buffer.
     */
    Bitmap(void* bits, stl::size_t byte_len, bool clear = true) noexcept;

    Bitmap(Bitmap&&) noexcept;

    Bitmap& operator=(Bitmap&&) noexcept;

    void swap(Bitmap&) noexcept;

    Bitmap& Init(void* bits, stl::size_t byte_len, bool clear = true) noexcept;

    /**
     * @brief Try to allocate the specified number of bits.
     *
     * @param count The number of bits to be allocated.
     * @return
     * The beginning index of the allocated bits if it succeeds.
     * Otherwise, @p npos.
     */
    stl::size_t Alloc(stl::size_t count = 1) noexcept;

    //! Forcefully mark the specified bits as allocated.
    Bitmap& ForceAlloc(stl::size_t begin, stl::size_t count = 1) noexcept;

    Bitmap& Free(stl::size_t begin, stl::size_t count = 1) noexcept;

    stl::size_t GetCapacity() const noexcept;

    stl::size_t GetByteLen() const noexcept;

    Bitmap& Clear() noexcept;

    const void* GetBits() const noexcept;

    bool IsAlloc(stl::size_t idx) const noexcept;

private:
    Bitmap& SetBit(stl::size_t idx, bool val) noexcept;

    Bitmap& Set(stl::size_t begin, stl::size_t count) noexcept;

    Bitmap& Reset(stl::size_t begin, stl::size_t count) noexcept;

    stl::size_t byte_len_ {0};
    stl::uint8_t* bits_ {nullptr};
};

void swap(Bitmap&, Bitmap&) noexcept;