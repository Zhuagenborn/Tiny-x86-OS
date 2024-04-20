; Segment selectors.

; The segment selector.
; It can identify a descriptor in a descriptor table.
; ```
;    15-3    2   1-0
; ┌───────┬────┬─────┐
; │ Index │ TI │ RPL │
; └───────┴────┴─────┘
;           ▲
;           └─ 0: The index is for the global descriptor table.
;              1: The index is for a local descriptor table.
; ```
; The selector's requested privilege level is 0.
sel_rpl_0       equ     0
; The selector's requested privilege level is 1.
sel_rpl_1       equ     1
; The selector's requested privilege level is 2.
sel_rpl_2       equ     2
; The selector's requested privilege level is 3.
sel_rpl_3       equ     3
; The selector is for the global descriptor table.
sel_ti_gdt      equ     0b000
; The selector is for a local descriptor table.
sel_ti_ldt      equ     0b100

; The selector for code.
sel_code        equ     (1 << 3) + sel_ti_gdt + sel_rpl_0
; The selector for data.
sel_data        equ     (2 << 3) + sel_ti_gdt + sel_rpl_0
; The selector for the VGA text buffer.
sel_video       equ     (3 << 3) + sel_ti_gdt + sel_rpl_0