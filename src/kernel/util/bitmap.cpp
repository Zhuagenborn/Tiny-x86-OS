#include "kernel/util/bitmap.h"
#include "kernel/debug/assert.h"
#include "kernel/stl/cstring.h"
#include "kernel/stl/utility.h"
#include "kernel/util/bit.h"
#include "kernel/util/metric.h"

Bitmap& Bitmap::Init(void* const bits, const stl::size_t byte_len, const bool clear) noexcept {
    dbg::Assert(bits && byte_len > 0);
    bits_ = static_cast<stl::uint8_t*>(bits);
    byte_len_ = byte_len;
    if (clear) {
        Clear();
    }

    return *this;
}

Bitmap::Bitmap(Bitmap&& o) noexcept {
    swap(o);
    o.bits_ = nullptr;
    o.byte_len_ = 0;
}

Bitmap& Bitmap::operator=(Bitmap&& o) noexcept {
    swap(o);
    o.bits_ = nullptr;
    o.byte_len_ = 0;
    return *this;
}

const void* Bitmap::GetBits() const noexcept {
    return bits_;
}

void Bitmap::swap(Bitmap& o) noexcept {
    using stl::swap;
    swap(bits_, o.bits_);
    swap(byte_len_, o.byte_len_);
}

stl::size_t Bitmap::GetCapacity() const noexcept {
    return byte_len_ * bit::byte_len;
}

stl::size_t Bitmap::GetByteLen() const noexcept {
    return byte_len_;
}

Bitmap::Bitmap(void* const bits, const stl::size_t byte_len, const bool clear) noexcept {
    Init(bits, byte_len, clear);
}

bool Bitmap::IsAlloc(const stl::size_t idx) const noexcept {
    dbg::Assert(bits_);
    const auto byte_idx {idx / bit::byte_len};
    const auto bit_idx {idx % bit::byte_len};
    dbg::Assert(byte_idx < byte_len_);
    return bit::IsBitSet(bits_[byte_idx], bit_idx);
}

Bitmap& Bitmap::Set(const stl::size_t begin, const stl::size_t count) noexcept {
    dbg::Assert(bits_);
    dbg::Assert(count > 0);
    for (stl::size_t i {0}; i != count; ++i) {
        SetBit(i + begin, true);
    }

    return *this;
}

Bitmap& Bitmap::Reset(const stl::size_t begin, const stl::size_t count) noexcept {
    dbg::Assert(bits_);
    dbg::Assert(count > 0);
    for (stl::size_t i {0}; i != count; ++i) {
        SetBit(i + begin, false);
    }

    return *this;
}

Bitmap& Bitmap::SetBit(const stl::size_t idx, const bool val) noexcept {
    dbg::Assert(bits_);
    const auto byte_idx {idx / bit::byte_len};
    const auto bit_idx {idx % bit::byte_len};
    dbg::Assert(byte_idx < byte_len_);
    if (val) {
        bit::SetBit(bits_[byte_idx], bit_idx);
    } else {
        bit::ResetBit(bits_[byte_idx], bit_idx);
    }

    return *this;
}

Bitmap& Bitmap::Free(const stl::size_t begin, const stl::size_t count) noexcept {
    return Reset(begin, count);
}

Bitmap& Bitmap::Clear() noexcept {
    stl::memset(bits_, 0, byte_len_);
    return *this;
}

Bitmap& Bitmap::ForceAlloc(const stl::size_t begin, const stl::size_t count) noexcept {
    return count > 0 ? Set(begin, count) : *this;
}

stl::size_t Bitmap::Alloc(const stl::size_t count) noexcept {
    dbg::Assert(bits_ && count > 0);
    stl::size_t byte_idx {0};
    while (byte_idx != byte_len_ && bits_[byte_idx] == 0xFF) {
        ++byte_idx;
    }

    if (byte_idx == byte_len_) {
        return npos;
    }

    stl::size_t bit_idx {0};
    while (bit_idx != bit::byte_len && bit::IsBitSet(bits_[byte_idx], bit_idx)) {
        ++bit_idx;
    }

    dbg::Assert(bit_idx < bit::byte_len);
    bit_idx += byte_idx * bit::byte_len;
    stl::size_t found_count {0};
    for (auto i {bit_idx}; i != byte_len_ * bit::byte_len; ++i) {
        if (!IsAlloc(i)) {
            if (++found_count == count) {
                const auto begin {i - count + 1};
                Set(begin, count);
                return begin;
            }
        } else {
            found_count = 0;
        }
    }

    return npos;
}

void swap(Bitmap& lhs, Bitmap& rhs) noexcept {
    lhs.swap(rhs);
}