; Descriptors.

%include "kernel/util/metric.inc"

; The descriptor table register.
struc       DescTabReg
    .limit:     resw    1
    .base:      resd    1
endstruc

; The size of a descriptor in bytes.
desc_size       equ     B(8)

; The segment descriptor.
; It is a part of memory segmentation, used for translating a logical address into a linear address.
; It describes the memory segment referred to in the logical address.
; ```
; --------------------------------------------- High 32 bits ---------------------------------------------
;      31-24    23   22   21   20       19-16     15  14-13  12  11-8       7-0
; ┌────────────┬───┬─────┬───┬─────┬─────────────┬───┬─────┬───┬──────┬────────────┐
; │ Base 31-24 │ G │ D/B │ L │ AVL │ Limit 19-16 │ P │ DPL │ S │ TYPE │ Base 23-16 │
; └────────────┴───┴─────┴───┴─────┴─────────────┴───┴─────┴───┴──────┴────────────┘
;                ▲    ▲    ▲                       ▲         ▲
;                │    │    │                       │         └─ 0: The segment is a system segment.
;                │    │    │                       │            1: The segment is a data or code segment.
;                │    │    │                       └─ 1: The segment presents.
;                │    │    └─ 0: The segment is 32-bit.
;                │    │       1: The segment is 64-bit.
;                │    └─ D (code segments): 0: The segment is 16-bit.
;                │                          1: The segment is 32-bit.
;                │       B (data segments): 0: The offset is 16-bit.
;                │                          1: The offset is 64-bit.
;                └─ 0: The limit is in units of bytes.
;                   1: The limit is in units of 4 KB.
; --------------------------------------------- Low 32 bits ---------------------------------------------
;     31-16        15-0
; ┌───────────┬────────────┐
; │ Base 15-0 │ Limit 15-0 │
; └───────────┴────────────┘
; ```
; The segment limit in units of 4 KB.
desc_g_4k       equ     0b1000_0000_0000_0000_0000_0000
; The segment is 32-bit.
desc_d_32       equ     0b100_0000_0000_0000_0000_0000
; The segment is 32-bit.
desc_l_32       equ     0
; This is not used by hardware.
desc_avl        equ     0
; The segment presents.
desc_p          equ     0b1000_0000_0000_0000
; The descriptor privilege level is 0.
desc_dpl_0      equ     0b000_0000_0000_0000
; The descriptor privilege level is 1.
desc_dpl_1      equ     0b010_0000_0000_0000
; The descriptor privilege level is 2.
desc_dpl_2      equ     0b100_0000_0000_0000
; The descriptor privilege level is 3.
desc_dpl_3      equ     0b110_0000_0000_0000
; The segment is a system segment.
desc_s_sys      equ     0b0_0000_0000_0000
; The segment is a code segment.
desc_s_code     equ     0b1_0000_0000_0000
; The segment is a data segment.
desc_s_data     equ     desc_s_code

; Code segments are executable, non-conforming, unreadable.
desc_type_code      equ     0b1000_0000_0000

; Data segments are non-executable, expand-up, non-conforming and writable.
desc_type_data      equ     0b0010_0000_0000

; The limit of code or data segments is `0xFFFFF`.
; They can span full 4 GB memory in 32-bit mode when the granularity flag is set.
desc_limit_code_16_19_bits      equ     0b1111_0000_0000_0000_0000
desc_limit_data_16_19_bits      equ     desc_limit_code_16_19_bits

; The limit of the VGA text buffer is `(0xBFFFF - 0xB8000) / 4 KB = 7`.
desc_limit_video_16_19_bits     equ     0

; Code segments range from `0x00000000` to `0xFFFFFFFF`.
desc_code_high_32_bits      equ     (0 << 24) + desc_g_4k + desc_d_32 + desc_l_32 \
                                    + desc_avl + desc_limit_code_16_19_bits + desc_p \
                                    + desc_dpl_0 + desc_s_code + desc_type_code + 0

; Data segments range from `0x00000000` to `0xFFFFFFFF`.
desc_data_high_32_bits      equ     (0 << 24) + desc_g_4k + desc_d_32 + desc_l_32 \
                                    + desc_avl + desc_limit_data_16_19_bits + desc_p \
                                    + desc_dpl_0 + desc_s_data + desc_type_data + 0

; The VGA text buffer ranges from `0x000B8000` to `0x00007FFF`.
desc_video_high_32_bits     equ     (0 << 24) + desc_g_4k + desc_d_32 + desc_l_32 \
                                    + desc_avl + desc_limit_video_16_19_bits + desc_p \
                                    + desc_dpl_0 + desc_s_data + desc_type_data + 0xB