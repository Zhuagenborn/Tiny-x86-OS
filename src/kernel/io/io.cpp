#include "kernel/io/io.h"

namespace io {

namespace {

extern "C" {

//! Get the value of @p EFLAGS.
stl::uint32_t GetEFlags() noexcept;

//! Set the value of @p EFLAGS.
stl::uint32_t SetEFlags(stl::uint32_t) noexcept;
}

}  // namespace

EFlags EFlags::Get() noexcept {
    return GetEFlags();
}

void EFlags::Set(const EFlags eflags) noexcept {
    SetEFlags(eflags);
}

}  // namespace io