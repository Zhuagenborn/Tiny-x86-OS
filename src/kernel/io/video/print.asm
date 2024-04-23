%include "kernel/util/metric.inc"
%include "kernel/io/video/print.inc"
%include "kernel/selector/sel.inc"

extern      EnableIntr
extern      DisableIntr
extern      IsIntrEnabled

; The backspace.
BS      equ     0x8
; The horizontal tab.
HT      equ     0x9
; The line feed.
LF      equ     0xA
; The carriage return.
CR      equ     0xD
; The end-of-file.
EOF     equ     -1
; The width of a horizontal tab.
tab_width   equ     2

[bits 32]
section     .text

global GetCursorPos
; Get the cursor position.
GetCursorPos:
    ; Get the high 8 bits.
    push    dx
    mov     dx, vga_ctrl_addr_port
    mov     al, vga_ctrl_cursor_loc_high_idx
    out     dx, al
    mov     dx, vga_ctrl_data_port
    in      al, dx
    mov     ah, al
    ; Get the low 8 bits.
    mov     dx, vga_ctrl_addr_port
    mov     al, vga_ctrl_cursor_loc_low_idx
    out     dx, al
    mov     dx, vga_ctrl_data_port
    in      al, dx
    pop     dx
    ret

global SetCursorPos
; Set the cursor position.
SetCursorPos:
    %push   set_cursor
    %stacksize  flat
    %arg    pos:word
        enter   B(0), 0
        pushad
        mov     bx, [pos]
        ; Set the high 8 bits.
        mov     dx, vga_ctrl_addr_port
        mov     al, vga_ctrl_cursor_loc_high_idx
        out     dx, al
        mov     dx, vga_ctrl_data_port
        mov     al, bh
        out     dx, al
        ; Set the low 8 bits.
        mov     dx, vga_ctrl_addr_port
        mov     al, vga_ctrl_cursor_loc_low_idx
        out     dx, al
        mov     dx, vga_ctrl_data_port
        mov     al, bl
        out     dx, al
        popad
        leave
        ret
    %pop

global      PrintChar
; Print a character.
PrintChar:
    %push   print_char
    %stacksize  flat
    %arg    ch:byte
        enter   B(0), 0
        mov     eax, [ch]
        ; Check if the character is a horizontal tab.
        cmp     eax, HT
        jne      .not_horizontal_tab
        mov     ecx, tab_width
    .print_space:
        push    ' '
        call    PrintChar
        add     esp, B(4)
        loop    .print_space
        jmp     .end

    .not_horizontal_tab:
        pushad
        mov     ax, sel_video
        mov     gs, ax

        call    GetCursorPos
        mov     bx, ax

        ; Get a character from the stack.
        mov     ecx, [ch]
        cmp     cl, CR
        je      .is_carriage_ret
        cmp     cl, LF
        je      .is_line_feed
        cmp     cl, BS
        je      .is_backspace
        jmp     .is_other

    .is_backspace:
        ; Delete the last character.
        dec     bx
        shl     bx, 1
        mov     byte [gs:bx], ' '
        shr     bx, 1
        jmp     .set_cursor

    .is_other:
        shl     bx, 1
        mov     [gs:bx], cl
        shr     bx, 1
        inc     bx
        ; Check if the screen needs to scroll down.
        cmp     bx, text_screen_width * text_screen_height
        jl      .set_cursor

    .is_line_feed:
    .is_carriage_ret:
        ; Set the cursor position to the beginning of the next line.
        xor     dx, dx
        mov     ax, bx
        mov     si, text_screen_width
        div     si
        sub     bx, dx
        add     bx, text_screen_width
        cmp     bx, text_screen_width * text_screen_height
        jl      .set_cursor

    .roll_screen:
        ; Move characters from lines `1`-`24` to `0`-`23`.
        call    IsIntrEnabled
        mov     edx, eax
        call    DisableIntr
        push    ds
        push    es
        ; After thread switching, the new thread will use incorrect `ES` and `DS`.
        ; So the interrupt must be disabled to prevent thread switching.
        mov     ax, gs
        mov     ds, ax
        mov     es, ax
        cld
        mov     ecx, text_screen_width * (text_screen_height - 1)
        lea     esi, [text_screen_width * B(2)]
        lea     edi, [0]
        rep     movsw
        pop     es
        pop     ds
        cmp     edx, False
        je      .intr_disabled
        call    EnableIntr
    .intr_disabled:
        ; Fill the last line with blanks.
        mov     ebx, (text_screen_width * (text_screen_height - 1)) * B(2)
        mov     ecx, text_screen_width
    .fill_blank:
        mov     byte [gs:ebx], ' '
        add     ebx, B(2)
        loop    .fill_blank
        mov     ebx, text_screen_width * (text_screen_height - 1)

    .set_cursor:
        push    ebx
        call    SetCursorPos
        add     esp, B(4)
        popad
    .end:
        leave
        ret
    %pop

global      PrintStr
; Print a string.
PrintStr:
    %push   print_str
    %stacksize  flat
    %arg    str:dword
        enter   B(0), 0
        push    ebx
        xor     ecx, ecx
        mov     ebx, [str]
    .print_char:
        mov     cl, [ebx]
        cmp     cl, 0
        je      .end
        push    ecx
        call    PrintChar
        add     esp, B(4)
        inc     ebx
        jmp     .print_char
    .end:
        pop     ebx
        leave
        ret
    %pop

global      PrintHex
; Print an unsigned hexadecimal integer without the prefix `0x`.
PrintHex:
    %push   print_hex
    %stacksize  flat
    %arg    n:dword
        enter   B(8), 0
        pushad

        ; Convert the integer to a string.
        mov     eax, [n]
        mov     edx, eax
        ; `EBX` is the address of a buffer for saving digits.
        lea     ebx, [ebp - B(8)]
        ; The first read digit should be saved at the end of the buffer.
        mov     edi, B(7)
        mov     ecx, 8      ; 8 digits in total.
    .read_digit:
        and     edx, 0xF
        cmp     edx, 9
        jg      .digit_is_a_to_f
        add     edx, '0'
        jmp     .save_digit
    .digit_is_a_to_f:
        sub     edx, 0xA
        add     edx, 'A'
    .save_digit:
        mov     [ebx + edi], dl
        dec     edi
        shr     eax, 4
        mov     edx, eax
        loop    .read_digit

        ; Find the first non-zero digit.
        xor     edi, edi
    .skip_prefix_0:
        cmp     edi, 8
        je      .start_print
        mov     cl, [ebx + edi]
        inc     edi
        cmp     cl, '0'
        je      .skip_prefix_0

        ; Print the string.
    .start_print:
        dec     edi
    .print_digit:
        mov     cl, [ebx + edi]
        push    ecx
        call    PrintChar
        add     esp, B(4)
        inc     edi
        cmp     edi, 8
        jl      .print_digit

        popad
        leave
        ret
    %pop