#pragma once

#include "kernel/stl/cstdint.h"

namespace stl {

class source_location {
public:
    static constexpr source_location current(const char* const file = __builtin_FILE(),
                                             const char* const func = __builtin_FUNCTION(),
                                             const uint_least32_t line = __builtin_LINE(),
                                             const uint_least32_t col = 0) noexcept {
        return {file, func, line, col};
    }

    constexpr const char* file_name() const noexcept {
        return file_;
    }

    constexpr const char* function_name() const noexcept {
        return func_;
    }

    constexpr uint_least32_t line() const noexcept {
        return line_;
    }

    constexpr uint_least32_t column() const noexcept {
        return col_;
    }

private:
    constexpr source_location(const char* const file, const char* const func,
                              const uint_least32_t line, const uint_least32_t col) noexcept :
        file_ {file}, func_ {func}, line_ {line}, col_ {col} {}

    const char* file_;
    const char* func_;
    const uint_least32_t line_;
    const uint_least32_t col_;
};

}  // namespace stl