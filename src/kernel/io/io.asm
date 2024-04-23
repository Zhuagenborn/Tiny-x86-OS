%include "kernel/util/metric.inc"

[bits 32]
section     .text
global      GetCr2
; Get the value of `CR2`.
GetCr2:
    mov     eax, cr2
    ret

global      SetCr3
; Set the value of `CR3`.
SetCr3:
    %push   set_cr3
    %stacksize  flat
    %arg    val:dword
        enter   B(0), 0
        mov     eax, [val]
        mov     cr3, eax
        leave
        ret
    %pop

global      GetEFlags
; Get the value of `EFLAGS`.
GetEFlags:
    pushfd
    pop     eax
    ret

global      SetEFlags
; Set the value of `EFLAGS`.
SetEFlags:
    %push   set_eflags
    %stacksize  flat
    %arg    val:dword
        enter   B(0), 0
        call    GetEFlags
        mov     edx, [val]
        push    edx
        popf
        leave
        ret
    %pop

global      WriteByteToPort
; Write a byte to a port.
WriteByteToPort:
    %push   write_byte_to_port
    %stacksize  flat
    %arg    port:word, val:byte
        enter   B(0), 0
        mov     dx, [port]
        mov     al, [val]
        out     dx, al
        leave
        ret
    %pop

global      WriteWordsToPort
; Write a number of words to a port.
WriteWordsToPort:
    %push   write_words_to_port
    %stacksize  flat
    %arg    port:word, data:dword, count:dword
        enter   B(0), 0
        push    esi
        cld
        mov     dx, [port]
        mov     esi, [data]
        mov     ecx, [count]
        rep     outsw
        pop     esi
        leave
        ret
    %pop

global      ReadByteFromPort
; Read a byte from a port.
ReadByteFromPort:
    %push   read_byte_from_port
    %stacksize  flat
    %arg    port:word
        enter   B(0), 0
        mov     dx, [port]
        in      al, dx
        leave
        ret
    %pop

global      ReadWordsFromPort
; Read a number of words from a port.
ReadWordsFromPort:
    %push   read_words_from_port
    %stacksize  flat
    %arg    port:word, buf:dword, count:dword
        enter   B(0), 0
        push    esi
        cld
        mov     dx, [port]
        mov     edi, [buf]
        mov     ecx, [count]
        rep     insw
        pop     esi
        leave
        ret
    %pop