; Basic kernel configurations.

%include "boot/boot.inc"
%include "kernel/memory/page.inc"

; The size of the kernel in bytes.
krnl_size           equ     MB(1)
; The first disk sector of the kernel.
krnl_start_sector   equ     loader_start_sector + loader_sector_count
; The size of the kernel in disk sectors.
krnl_sector_count   equ     350
; The buffer address for reading the raw kernel from the file.
raw_krnl_base       equ     0x00070000
; The address of the kernel image when it is loaded.
krnl_base           equ     0xC0000000
; The stack top address of the kernel.
krnl_stack_top      equ     krnl_base + 0x0009F000
; The code entry address of the kernel.
krnl_code_entry     equ     krnl_base + 0x00001500

; The address of the page directory table.
page_dir_base           equ     krnl_size
; The index of the first kernel page directory entry.
krnl_page_dir_start     equ     (krnl_base & 0xFFC00000) >> 22
; The number of kernel page directory entries.
krnl_page_dir_count     equ     page_dir_count - krnl_page_dir_start - 1