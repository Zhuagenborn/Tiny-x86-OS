%include "kernel/util/metric.inc"

; This structure must be the same as the beginning of `tsk::Thread`.
struc       Thread
    .tags:          resq    2
    .krnl_stack:    resd    1
    ; ...
endstruc

[bits 32]
section     .text
global      HaltCpu
; Halt the CPU until the next external interrupt is fired.
HaltCpu:
    hlt
    ret

global      GetCurrThread
; Get the current running thread.
; The size of a thread block is one memory page.
; The stack is located in the thread block. So the page address of `ESP` is the thread address.
;
; Note that the compiler might allocate more memory for the control block in a thread, especially with debug options.
; In that case, the size of a thread block is larger than one memory page,
; and this method no longer works.
GetCurrThread:
    mov     eax, esp
    and     eax, 0xFFFFF000
    ret

global      SwitchThread
; Switch the running thread.
; ```c++
; void SwitchThread(Thread& from, Thread& to) noexcept;
; ```
SwitchThread:
    ; The stack top is the return address of the current thread.
    ; When it is scheduled next time, it will continue to run from that address.

    ; Save the remaining registers as the structure `tsk::Thread::SwitchStack`.
    push    esi
    push    edi
    push    ebx
    push    ebp
    ; Get the current thread from the first argument.
    mov     eax, [esp + B(20)]
    ; Save the stack address to the current thread block.
    mov     [eax + Thread.krnl_stack], esp

    ; Get the new thread from the second argument.
    mov     eax, [esp + B(24)]
    ; Get the new stack address from the new thread block.
    mov     esp, [eax + Thread.krnl_stack]
    ; Restore the registers of the new thread.
    pop     ebp
    pop     ebx
    pop     edi
    pop     esi
    ; Pop the return address of the new thread and continue to run it.
    ret