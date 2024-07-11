%include "kernel/util/metric.inc"

; The interrupt number of system calls.
sys_call_intr_num       equ     0x30

[bits 32]
section     .text
global      SysCall
; The system call.
; It accepts an index and an optional user-defined argument,
; then calls the indexed method with the argument in the system call interrupt.
; Developers should register kernel APIs as system calls first before calling them in user mode.
SysCall:
    %push   sys_call
    %stacksize  flat
    %arg    func:dword, arg:dword
        enter   B(0), 0
        mov     ecx, [func]
        mov     eax, [arg]
        ; Jump to the method `SysCallEntry` in `src/kernel/interrupt/intr.asm`.
        int     sys_call_intr_num
        leave
        ret
    %pop