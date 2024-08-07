#include "kernel/stl/cstdlib.h"
#include "kernel/thread/thd.h"

int main() {
    InitKernel();

    while (true) {
        tsk::Thread::GetCurrent().Yield();
    }

    return stl::EXIT_SUCCESS;
}