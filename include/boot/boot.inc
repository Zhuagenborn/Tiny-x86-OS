; Basic configurations of the master boot record and the kernel loader.

%include "kernel/util/metric.inc"

; The physical address of the master boot record, specified by the BIOS.
mbr_base            equ     0x7C00
; The physical stack top address of the master boot record.
mbr_stack_top       equ     mbr_base

; The physical address of the loader.
loader_base             equ     0x900
; The size of the data at the beginning of the loader, mainly global descriptors.
loader_data_size        equ     B(0x400)
; The physical code entry address of the loader.
loader_code_entry       equ     loader_base + loader_data_size
; The first disk sector of the loader (the sector `0` is for the master boot record).
loader_start_sector     equ     1
; The size of the loader in disk sectors.
loader_sector_count     equ     5
; The physical stack top address of the loader.
loader_stack_top        equ     loader_base