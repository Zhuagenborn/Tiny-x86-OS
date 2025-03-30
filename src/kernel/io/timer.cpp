#include "kernel/io/timer.h"
#include "kernel/debug/assert.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/io.h"
#include "kernel/io/video/print.h"
#include "kernel/thread/thd.h"
#include "kernel/util/bit.h"

namespace io {

namespace {

/**
 * @brief A wrapper of a global @p bool variable representing whether the timer has been initialized.
 *
 * @details
 * Global variables cannot be initialized properly.
 * We use @p static variables in methods instead since they can be initialized by methods.
 */
bool& IsTimerInitedImpl() noexcept {
    static bool inited {false};
    return inited;
}

//! *Intel 8253* registers.
namespace port {
//! The data register for the counter @p 0.
inline constexpr stl::uint16_t counter_0 {0x40};
//! The data register for the counter @p 1.
inline constexpr stl::uint16_t counter_1 {0x41};
//! The data register for the counter @p 2.
inline constexpr stl::uint16_t counter_2 {0x42};
//! The mode and command register.
static constexpr stl::uint16_t pit_ctrl {0x43};
}  // namespace port

//! The number of ticks after system startup.
stl::size_t ticks {0};

enum class ReadWriteMode {
    LatchRead = 0,
    ReadWriteLowByte = 1,
    ReadWriteHighByte = 2,
    ReadWriteLowHighBytes = 3
};

enum class CountMode {
    IntrOnTerminalCount = 0,
    HardRetriggerOneShot = 1,
    RateGenerator = 2,
    SquareWaveGenerator = 3,
    SoftTriggerStrobe = 4,
    HardTriggerStrobe = 5,
};

enum class DigitalMode { Binary, BinaryCodedDecimal };

//! The control word.
class CtrlWord {
public:
    constexpr CtrlWord(const stl::uint8_t val = 0) noexcept : val_ {val} {}

    constexpr operator stl::uint8_t() const noexcept {
        return val_;
    }

    constexpr CtrlWord& SetDigitalMode(const DigitalMode mode) noexcept {
        if (mode == DigitalMode::BinaryCodedDecimal) {
            bit::SetBit(val_, bcd_pos);
        } else {
            bit::ResetBit(val_, bcd_pos);
        }

        return *this;
    }

    constexpr CtrlWord& SetCountMode(const CountMode mode) noexcept {
        bit::SetBits(val_, static_cast<stl::uint32_t>(mode), m_pos, m_len);
        return *this;
    }

    constexpr CtrlWord& SetReadWriteMode(const ReadWriteMode mode) noexcept {
        bit::SetBits(val_, static_cast<stl::uint32_t>(mode), rw_pos, rw_len);
        return *this;
    }

    CtrlWord& SetSelectCounter(const stl::size_t id) noexcept {
        dbg::Assert(id < 3);
        bit::SetBits(val_, id, sc_pos, sc_len);
        return *this;
    }

    void WriteToPort() noexcept {
        io::WriteByteToPort(port::pit_ctrl, val_);
    }

private:
    static constexpr stl::size_t bcd_pos {0};
    static constexpr stl::size_t m_pos {bcd_pos + 1};
    static constexpr stl::size_t m_len {3};
    static constexpr stl::size_t rw_pos {m_pos + m_len};
    static constexpr stl::size_t rw_len {2};
    static constexpr stl::size_t sc_pos {rw_pos + rw_len};
    static constexpr stl::size_t sc_len {2};

    stl::uint8_t val_;
};

//! Calculate the initial counter value by a timer interrupt frequency.
constexpr stl::uint32_t CalcInitCounterVal(const stl::size_t freq_per_second) noexcept {
    constexpr stl::size_t input_freq {1193180};
    return input_freq / freq_per_second;
}

void InitCounter(const stl::size_t freq_per_second) noexcept {
    CtrlWord {}
        .SetSelectCounter(0)
        .SetCountMode(CountMode::RateGenerator)
        .SetReadWriteMode(ReadWriteMode::ReadWriteLowHighBytes)
        .SetDigitalMode(DigitalMode::Binary)
        .WriteToPort();

    const auto init_val {CalcInitCounterVal(freq_per_second)};
    io::WriteByteToPort(port::counter_0, bit::GetLowByte(init_val));
    io::WriteByteToPort(port::counter_0, bit::GetHighByte(init_val));
}

/**
 * @brief The clock interrupt handler.
 *
 * @details
 * It increases ticks and schedules threads.
 */
void ClockIntrHandler(stl::size_t) noexcept {
    auto& curr_thd {tsk::Thread::GetCurrent()};
    dbg::Assert(curr_thd.IsStackValid());
    ++ticks;
    if (!curr_thd.Tick()) {
        curr_thd.Schedule();
    }
}

}  // namespace

stl::size_t GetTicks() noexcept {
    return ticks;
}

void InitTimer(const stl::size_t freq_per_second) noexcept {
    dbg::Assert(!IsTimerInited());
    dbg::Assert(tsk::IsThreadInited());
    ticks = 0;
    InitCounter(freq_per_second);
    intr::GetIntrHandlerTab().Register(intr::Intr::Clock, &ClockIntrHandler);
    IsTimerInitedImpl() = true;
    io::PrintStr("Intel 8253 Programmable Interval Timer has been initialized.\n");
}

bool IsTimerInited() noexcept {
    return IsTimerInitedImpl();
}

}  // namespace io