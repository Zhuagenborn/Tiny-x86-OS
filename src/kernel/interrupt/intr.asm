%include "kernel/util/metric.inc"

; Interrupt handlers, defined in `src/interrupt/intr.cpp`.
; ```c++
; void Handler(std::size_t intr_num) noexcept;
; ```
extern      intr_handlers

; System call handlers, defined in `src/syscall/call.cpp`.
; Developers should register kernel APIs here so they can be called in user mode.
; ```c++
; std::int32_t Handler(void* arg) noexcept;
; ```
extern      sys_call_handlers

; The command register for the master Intel 8259A chip.
pic_master_cmd_port     equ     0x20
; The data register for the master Intel 8259A chip.
pic_master_data_port    equ     0x21
; The command register for the slave Intel 8259A chip.
pic_slave_cmd_port      equ     0xA0
; The data register for the slave Intel 8259A chip.
pic_slave_data_port     equ     0xA1

; The end-of-interrupt signal.
pic_op_cmd_word2_eoi    equ     0b0010_0000

; Some interrupts have an error code.
; It is pushed onto the stack by the CPU when the interrupt occurs.
; To simplify the code, we push a zero if the CPU does not push an error code.
%define     error_code      nop
%define     zero            push    0

; Create an entry point of an interrupt handler.
; `%1`: An interrupt number.
; `%2`: `error_code` if the interrupt has an error code. Otherwise `zero`.
%macro      intr_entry 2
section     .text
Intr%1HandlerEntry:
    ; Some segment and flag registers are already pushed onto the stack by the CPU.
    ; Push a zero if the CPU does not push an error code.
    %2

    ; Save other registers.
    ; See the structure `IntrStack` in `include/kernel/interrupt/intr.h` for details.
    push    ds
    push    es
    push    fs
    push    gs
    pushad

    ; Send end-of-interrupt signals.
    mov     al, pic_op_cmd_word2_eoi
    out     pic_slave_cmd_port, al
    out     pic_master_cmd_port, al

    ; Push the interrupt number.
    push    %1
    call    [intr_handlers + %1 * B(4)]
    jmp     intr_exit

section     .data
    ; The address of the entry point.
    ; They will be merged into the array `intr_entries` by the compiler.
    dd      Intr%1HandlerEntry
%endmacro

[bits 32]
section     .data
global      intr_entries
; The entry points of interrupts.
; They will be registered in the interrupt descriptor table.
; When an interrupt occurs, the CPU jumps to the corresponding entry point.
; The interrupt handler is called after registers are saved.
intr_entries:
    intr_entry  0x00, zero
    intr_entry  0x01, zero
    intr_entry  0x02, zero
    intr_entry  0x03, zero
    intr_entry  0x04, zero
    intr_entry  0x05, zero
    intr_entry  0x06, zero
    intr_entry  0x07, zero
    intr_entry  0x08, error_code
    intr_entry  0x09, zero
    intr_entry  0x0A, error_code
    intr_entry  0x0B, error_code
    intr_entry  0x0C, zero
    intr_entry  0x0D, error_code
    intr_entry  0x0E, error_code
    intr_entry  0x0F, zero
    intr_entry  0x10, zero
    intr_entry  0x11, error_code
    intr_entry  0x12, zero
    intr_entry  0x13, zero
    intr_entry  0x14, zero
    intr_entry  0x15, zero
    intr_entry  0x16, zero
    intr_entry  0x17, zero
    intr_entry  0x18, error_code
    intr_entry  0x19, zero
    intr_entry  0x1A, error_code
    intr_entry  0x1B, error_code
    intr_entry  0x1C, zero
    intr_entry  0x1D, error_code
    intr_entry  0x1E, error_code
    intr_entry  0x1F, zero
    intr_entry  0x20, zero
    intr_entry  0x21, zero
    intr_entry  0x22, zero
    intr_entry  0x23, zero
    intr_entry  0x24, zero
    intr_entry  0x25, zero
    intr_entry  0x26, zero
    intr_entry  0x27, zero
    intr_entry  0x28, zero
    intr_entry  0x29, zero
    intr_entry  0x2A, zero
    intr_entry  0x2B, zero
    intr_entry  0x2C, zero
    intr_entry  0x2D, zero
    intr_entry  0x2E, zero
    intr_entry  0x2F, zero

; `0x30` is the interrupt number of system calls.
sys_call_intr_num       equ     0x30

section     .text
; The entry point of system calls (The method `SysCall` in `src/kernel/syscall/call.asm`).
; `ECX` = The index of an system call.
; `EAX` = A user-defined argument.
SysCallEntry:
    ; Some segment and flag registers are already pushed onto the stack by the CPU.
    push    0
    ; Save registers.
    push    ds
    push    es
    push    fs
    push    gs
    pushad
    push    sys_call_intr_num

    ; Call a handler.
    push    eax
    call    [sys_call_handlers + ecx * B(4)]
    add     esp, B(4)
    ; Copy the `EAX` value to the position of `EAX` in the saved register context,
    ; so users can get the return value after registers are restored.
    mov     [esp + B(4) * 8], eax
    jmp     intr_exit

section     .data
    dd      SysCallEntry

section     .text
global      intr_exit
intr_exit:
    ; Ignore the interrupt number.
    add     esp, B(4)
    ; Restore registers.
    popad
    pop     gs
    pop     fs
    pop     es
    pop     ds
    ; Ignore the error code.
    add     esp, B(4)
    iret

global      SetIntrDescTabReg
; Set the interrupt descriptor table register.
; ```c++
; void SetIntrDescTabReg(std::uint16_t limit, std::uintptr_t base) noexcept;
; ```
SetIntrDescTabReg:
    mov     ax, [esp + B(4)]
    mov     [esp + B(6)], ax
    lidt    [esp + B(6)]
    ret

global      JmpToIntrExit
; Jump to the exit of interrupt routines.
; The content of the interrupt stack will be restored to registers.
; ```c++
; [[noreturn]] void JmpToIntrExit(const void* intr_stack) noexcept;
; ```
JmpToIntrExit:
    ; Set `ESP` to the address of the interrupt stack.
    mov     eax, [esp + B(4)]
    mov     esp, eax
    jmp     intr_exit

global      GetIntrDescTabReg
; Get the interrupt descriptor table register.
GetIntrDescTabReg:
    %push   get_intr_desc_tab_reg
    %stacksize  flat
    %arg    reg_base:dword
        enter   B(0), 0
        mov     eax, [reg_base]
        sidt    [eax]
        leave
        ret
    %pop

global      EnableIntr
; Enable interrupts.
EnableIntr:
    sti
    ret

global      DisableIntr
; Disable interrupts.
DisableIntr:
    cli
    ret