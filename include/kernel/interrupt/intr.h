/**
 * @file intr.h
 * @brief The interrupt.
 *
 * @par GitHub
 * https://github.com/Zhuagenborn
 */

#pragma once

#include "kernel/debug/assert.h"
#include "kernel/descriptor/desc.h"
#include "kernel/stl/string_view.h"
#include "kernel/stl/utility.h"

namespace intr {

//! The number of interrupts.
inline constexpr stl::size_t count {0x31};
//! The interrupt number of the first user-defined interrupt.
inline constexpr stl::size_t start_usr_intr_num {0x20};
//! The maximum length of an interrupt hanlder's name.
inline constexpr stl::size_t max_intr_handler_name_len {32};

//! Interrupt numbers.
enum class Intr {
    //! The memory page fault.
    PageFault = 0x0E,

    /* Interrupts of Intel 8259A Programmable Interrupt Controller */
    //! The clock.
    Clock = start_usr_intr_num,
    //! The keyboard.
    Keyboard,
    //! The primary IDE channel.
    PrimaryIdeChnl = start_usr_intr_num + 14,
    //! The secondary IDE channel.
    SecondaryIdeChnl = start_usr_intr_num + 15,
    /********************************************************************/

    //! The system call.
    SysCall = count - 1
};

//! The interrupt descriptor table.
template <stl::size_t count>
class IntrDescTab : public desc::DescTabArray<desc::GateDesc, count> {
public:
    using desc::DescTabArray<desc::GateDesc, count>::operator[];

    const desc::GateDesc& operator[](const Intr intr) const noexcept {
        return this->descs_[static_cast<stl::size_t>(intr)];
    }

    desc::GateDesc& operator[](const Intr intr) noexcept {
        return const_cast<desc::GateDesc&>(const_cast<const IntrDescTab&>(*this)[intr]);
    }
};

//! The interrupt handler.
using Handler = void (*)(stl::size_t) noexcept;

/**
 * @brief The interrupt handler table.
 *
 * @details
 * It can be regarded as a manager for an array of function pointers.
 */
template <stl::size_t count>
class IntrHandlerTab {
    static_assert(count > 0);

public:
    /**
     * @brief Create an interrupt handler table.
     *
     * @param handlers An array of interrupt handlers.
     * @param default_name The default interrupt handler name.
     * @param default_handler The default interrupt handler.
     */
    IntrHandlerTab(Handler (&handlers)[count], const stl::string_view default_name,
                   const Handler default_handler) noexcept :
        handlers_ {handlers} {
        for (stl::size_t i {0}; i != count; ++i) {
            Register(i, default_name, default_handler);
        }
    }

    IntrHandlerTab& Register(const stl::size_t idx, const stl::string_view name) noexcept {
        dbg::Assert(idx < count);
        if (!name.empty()) {
            stl::strcpy_s(names_[idx].data(), max_intr_handler_name_len + 1, name.data());
        } else {
            names_[idx].front() = '\0';
        }

        return *this;
    }

    IntrHandlerTab& Register(const stl::size_t idx, const Handler handler) noexcept {
        dbg::Assert(idx < count);
        handlers_[idx] = handler;
        return *this;
    }

    IntrHandlerTab& Register(const stl::size_t idx, const stl::string_view name,
                             const Handler handler) noexcept {
        Register(idx, name);
        return Register(idx, handler);
    }

    IntrHandlerTab& Register(const Intr intr, const stl::string_view name) noexcept {
        return Register(static_cast<stl::size_t>(intr), name);
    }

    IntrHandlerTab& Register(const Intr intr, const Handler handler) noexcept {
        return Register(static_cast<stl::size_t>(intr), handler);
    }

    IntrHandlerTab& Register(const Intr intr, const stl::string_view name,
                             const Handler handler) noexcept {
        return Register(static_cast<stl::size_t>(intr), name, handler);
    }

    stl::string_view GetName(const stl::size_t idx) const noexcept {
        dbg::Assert(idx < count);
        return names_[idx].data();
    }

    constexpr stl::size_t GetCount() const noexcept {
        return count;
    }

    constexpr const Handler* GetHandlers() const noexcept {
        return handlers_;
    }

private:
    stl::array<stl::array<char, max_intr_handler_name_len + 1>, count> names_;
    Handler* handlers_ {nullptr};
};

/**
 * @brief The interrupt stack.
 *
 * @details
 * When an interrupt occurs, these values are pushed onto the stack.
 * Some values are automatically pushed by the CPU.
 * Some values are manually pushed by the kernel in the macro @p intr_entry in @p src/kernel/interrupt/intr.asm.
 *
 * @see @p src/kernel/interrupt/intr.asm
 */
struct IntrStack {
    /* These values are manually pushed by the kernel */
    stl::uint32_t intr_num;
    stl::uint32_t edi;
    stl::uint32_t esi;
    stl::uint32_t ebp;
    stl::uint32_t esp;
    stl::uint32_t ebx;
    stl::uint32_t edx;
    stl::uint32_t ecx;
    stl::uint32_t eax;
    stl::uint32_t gs;
    stl::uint32_t fs;
    stl::uint32_t es;
    stl::uint32_t ds;

    /* These values are automatically pushed by the CPU */
    /**
     * @details
     * Some interrupts do not have an error code.
     * To simplify the code, we push a zero when those interrupts occur.
     */
    stl::uint32_t err_code;
    stl::uint32_t old_eip;
    stl::uint32_t old_cs;
    stl::uint32_t eflags;
    stl::uint32_t old_esp;
    stl::uint32_t old_ss;
};

extern "C" {
//! Enable interrupts.
void EnableIntr() noexcept;

//! Disable interrupts.
void DisableIntr() noexcept;

//! Whether interrupts are enabled
bool IsIntrEnabled() noexcept;
}

//! Initialize interrupts.
void InitIntr() noexcept;

/**
 * @brief The interrupt guard.
 *
 * @details
 * It provides a convenient RAII-style mechanism for disabling interrupts for the duration of a scoped block.
 */
class IntrGuard {
public:
    //! Disable interrupts.
    IntrGuard() noexcept;

    //! Restore the original interrupt state.
    ~IntrGuard() noexcept;

private:
    bool enabled_;
};

/**
 * @brief The default interrupt handler.
 *
 * @details
 * It prints interrupt information and pauses the system.
 */
void DefaultIntrHandler(stl::size_t intr_num) noexcept;

//! Get the interrupt descriptor table register.
desc::DescTabReg GetIntrDescTabReg() noexcept;

//! Get the interrupt handler table.
IntrHandlerTab<count>& GetIntrHandlerTab() noexcept;

}  // namespace intr