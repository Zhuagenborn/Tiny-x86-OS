%include "kernel/util/metric.inc"

[bits 32]
section     .text
global      GetGlobalDescTabReg
; Get the global descriptor table register.
GetGlobalDescTabReg:
    %push   get_global_desc_tab_reg
    %stacksize  flat
    %arg    reg_base:dword
        enter   B(0), 0
        mov     eax, [reg_base]
        sgdt    [eax]
        leave
        ret
    %pop

global      SetGlobalDescTabReg
; Set the global descriptor table register.
; ```c++
; void SetGlobalDescTabReg(std::uint16_t limit, std::uintptr_t base) noexcept;
; ```
SetGlobalDescTabReg:
    mov     ax, [esp + B(4)]
    mov     [esp + B(6)], ax
    lgdt    [esp + B(6)]
    ret