; Video configurations.

; The screen width in VGA text mode.
text_screen_width       equ     80
; The screen height in VGA text mode.
text_screen_height      equ     25

; The VGA address register.
; The index of a data register should be saved in this register before accessing the data.
vga_ctrl_addr_port      equ     0x3D4
; The VGA data register.
vga_ctrl_data_port      equ     0x3D5

; The index of the register for the high bits of the cursor location.
vga_ctrl_cursor_loc_high_idx    equ     0xE
; The index of the register for the low bits of the cursor location.
vga_ctrl_cursor_loc_low_idx     equ     0xF