#include "kernel/interrupt/pic.h"
#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/io.h"
#include "kernel/io/video/print.h"
#include "kernel/util/bit.h"

namespace intr::pic {

namespace {

//! *Intel 8259A* registers.
namespace port {
//! The command register for the master *Intel 8259A* chip.
inline constexpr stl::uint16_t master_cmd {0x20};
//! The data register for the master *Intel 8259A* chip.
inline constexpr stl::uint16_t master_data {0x21};
//! The command register for the slave *Intel 8259A* chip.
inline constexpr stl::uint16_t slave_cmd {0xA0};
//! The data register for the slave *Intel 8259A* chip.
inline constexpr stl::uint16_t slave_data {0xA1};
}  // namespace port

//! The number of interrupt lines on an *Intel 8259A* chip.
inline constexpr stl::size_t irq_count {8};
//! The interrupt request for cascade.
inline constexpr stl::size_t cascade_irq {static_cast<stl::size_t>(Intr::SlavePic)};

//! The first interrupt number on the master *Intel 8259A* chip.
inline constexpr stl::size_t master_start_intr_num {static_cast<stl::size_t>(intr::Intr::Clock)};
//! The first interrupt number on the slave *Intel 8259A* chip.
inline constexpr stl::size_t slave_start_intr_num {master_start_intr_num + irq_count};

enum class TriggerMode { Level, Edge };

/**
 * @brief The command word.
 *
 * @details
 * *Intel 8259A* has two groups of command words:
 * - Initialization Command Words for device settings.
 * - Operating Command Words for interrupt settings.
 */
class CmdWord {
public:
    constexpr CmdWord(const stl::uint8_t val = 0) noexcept : val_ {val} {}

    constexpr operator stl::uint8_t() const noexcept {
        return val_;
    }

    //! Write the value of the command word to a port.
    void WriteToPort(const stl::uint16_t port) noexcept {
        io::WriteByteToPort(port, val_);
    }

protected:
    stl::uint8_t val_;
};

static_assert(sizeof(CmdWord) == sizeof(stl::uint8_t));

/**
 * Initialization Command Word 1.
 */
class InitCmdWord1 : public CmdWord {
public:
    constexpr InitCmdWord1(const stl::uint8_t val = 0) noexcept : CmdWord {val} {
        bit::SetBit(val_, 4);
    }

    //! Set cascade or a single *Intel 8259A* chip.
    constexpr InitCmdWord1& SetSingle(const bool single = true) noexcept {
        if (single) {
            bit::SetBit(val_, sngl_pos);
        } else {
            bit::ResetBit(val_, sngl_pos);
        }

        return *this;
    }

    //! Whether to set Initialization Command Word 4.
    constexpr InitCmdWord1& SetInitCmdWord4(const bool enable = true) noexcept {
        if (enable) {
            bit::SetBit(val_, ic4_pos);
        } else {
            bit::ResetBit(val_, ic4_pos);
        }

        return *this;
    }

    constexpr InitCmdWord1& SetTriggerMode(const TriggerMode mode) noexcept {
        if (mode == TriggerMode::Level) {
            bit::SetBit(val_, ltim_pos);
        } else {
            bit::ResetBit(val_, ltim_pos);
        }

        return *this;
    }

private:
    static constexpr stl::size_t ic4_pos {0};
    static constexpr stl::size_t sngl_pos {ic4_pos + 1};
    static constexpr stl::size_t ltim_pos {3};
};

/**
 * Initialization Command Word 2.
 */
class InitCmdWord2 : public CmdWord {
public:
    using CmdWord::CmdWord;

    //! Set the interrupt number for the interrupt request @p 0.
    InitCmdWord2& SetIrq0IntrNum(const stl::size_t num) noexcept {
        val_ = static_cast<stl::uint8_t>(num);
        bit::ResetBits(val_, id_pos, id_len);
        dbg::Assert(val_ == num);
        return *this;
    }

private:
    static constexpr stl::size_t id_pos {0};
    static constexpr stl::size_t id_len {3};
};

/**
 * Initialization Command Word 3 for the master *Intel 8259A* chip.
 */
class MasterInitCmdWord3 : public CmdWord {
public:
    using CmdWord::CmdWord;

    //! Set an interrupt request for cascade.
    MasterInitCmdWord3& AddCascadeIrq(const stl::size_t irq) noexcept {
        dbg::Assert(irq < irq_count);
        bit::SetBit(val_, irq);
        return *this;
    }
};

/**
 * Initialization Command Word 3 for the slave *Intel 8259A* chip.
 */
class SlaveInitCmdWord3 : public CmdWord {
public:
    using CmdWord::CmdWord;

    //! Set an interrupt request for cascade.
    SlaveInitCmdWord3& SetCascadeIrq(const stl::size_t irq) noexcept {
        dbg::Assert(irq < irq_count);
        val_ = irq;
        return *this;
    }
};

/**
 * Initialization Command Word 4.
 */
class InitCmdWord4 : public CmdWord {
public:
    using CmdWord::CmdWord;

    constexpr InitCmdWord4& Set8086(const bool enable = true) noexcept {
        if (enable) {
            bit::SetBit(val_, x86_pos);
        } else {
            bit::ResetBit(val_, x86_pos);
        }

        return *this;
    }

    /**
     * @brief Enable the auto end of interrupts.
     *
     * @details
     * *Intel 8259A* will not continue to process the next interrupt until it receives an end-of-interrupt signal.
     * If this option is enabled, the chip automatically ends the interrupt.
     * Otherwise, we have to manually send end-of-interrupt signal to the master and slave chips.
     */
    constexpr InitCmdWord4& SetAutoIntrEnd(const bool enable = true) noexcept {
        if (enable) {
            bit::SetBit(val_, aeoi_pos);
        } else {
            bit::ResetBit(val_, aeoi_pos);
        }

        return *this;
    }

private:
    static constexpr stl::size_t x86_pos {0};
    static constexpr stl::size_t aeoi_pos {x86_pos + 1};
};

/**
 * Operation Command Word 1.
 */
class OpCmdWord1 : public CmdWord {
public:
    using CmdWord::CmdWord;

    OpCmdWord1& EnableAllIntrs() noexcept {
        val_ = 0;
        return *this;
    }

    OpCmdWord1& DisableAllIntrs() noexcept {
        val_ = static_cast<stl::uint8_t>(-1);
        return *this;
    }

    OpCmdWord1& EnableIntr(const Intr intr) noexcept {
        return SetIntr(intr);
    }

    OpCmdWord1& DisableIntr(const Intr intr) noexcept {
        return SetIntr(intr, false);
    }

private:
    OpCmdWord1& SetIntr(const Intr intr, const bool enable = true) noexcept {
        const auto irq {static_cast<stl::size_t>(intr) % irq_count};
        if (enable) {
            bit::ResetBit(val_, irq);
        } else {
            bit::SetBit(val_, irq);
        }

        return *this;
    }
};

//! Initialize the master *Intel 8259A* chip.
void InitMaster() noexcept {
    InitCmdWord1 {}
        .SetTriggerMode(TriggerMode::Edge)
        .SetInitCmdWord4()
        .WriteToPort(port::master_cmd);

    InitCmdWord2 {}.SetIrq0IntrNum(master_start_intr_num).WriteToPort(port::master_data);

    MasterInitCmdWord3 {}.AddCascadeIrq(cascade_irq).WriteToPort(port::master_data);

    InitCmdWord4 {}.Set8086().WriteToPort(port::master_data);
}

//! Initialize the slave *Intel 8259A* chip.
void InitSlave() noexcept {
    InitCmdWord1 {}
        .SetTriggerMode(TriggerMode::Edge)
        .SetInitCmdWord4()
        .WriteToPort(port::slave_cmd);

    InitCmdWord2 {}.SetIrq0IntrNum(slave_start_intr_num).WriteToPort(port::slave_data);

    SlaveInitCmdWord3 {}.SetCascadeIrq(cascade_irq).WriteToPort(port::slave_data);

    InitCmdWord4 {}.Set8086().WriteToPort(port::slave_data);
}

//! Whether an interrupt is from the master *Intel 8259A* chip.
bool IsMasterIntr(const Intr intr) noexcept {
    return static_cast<stl::size_t>(intr) < irq_count;
}

}  // namespace

void InitPgmIntrCtrl(const stl::span<Intr> intrs) noexcept {
    InitMaster();
    InitSlave();

    // Disable all interrupts first and enable only the ones we want.
    OpCmdWord1 master_ocw, slave_ocw;
    master_ocw.DisableAllIntrs();
    slave_ocw.DisableAllIntrs();
    for (const auto intr : intrs) {
        if (IsMasterIntr(intr)) {
            master_ocw.EnableIntr(intr);
        } else {
            slave_ocw.EnableIntr(intr);
        }
    }

    master_ocw.WriteToPort(port::master_data);
    slave_ocw.WriteToPort(port::slave_data);

    io::PrintlnStr("Intel 8259A Programmable Interrupt Controller has been initialized.");
}

}  // namespace intr::pic