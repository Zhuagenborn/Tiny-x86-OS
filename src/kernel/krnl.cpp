#include "kernel/krnl.h"
#include "kernel/interrupt/intr.h"
#include "kernel/io/disk/disk.h"
#include "kernel/io/keyboard.h"
#include "kernel/io/timer.h"
#include "kernel/memory/pool.h"
#include "kernel/process/tss.h"
#include "kernel/syscall/call.h"
#include "kernel/thread/thd.h"

void InitKernel() noexcept {
    intr::InitIntr();
    sc::InitSysCall();
    mem::InitMem();
    tsk::InitThread();
    io::InitTimer(io::timer_freq_per_second);
    tsk::InitTaskStateSeg();
    io::InitKeyboard();
    intr::EnableIntr();
    io::InitDisk();
    io::InitFileSys();
}