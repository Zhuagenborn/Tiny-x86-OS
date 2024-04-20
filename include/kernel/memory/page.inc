; Memory paging.

%include "kernel/util/metric.inc"

; The page table entry and page directory entry.
; They are a part of memory paging, which allows each process to see a full virtual address space,
; without actually requiring the full amount of physical memory to be available.
; Memory paging is based on memory segmentation, it translates the linear address obtained from segmentation into a physical address.
; In the page directory, each entry points to a page table.
; In the page table, each entry points to a 4 KB physical page frame.
; ```
;      31-12    11-9   8   7   6   5    4     3     2    1   0
; ┌────────────┬─────┬───┬───┬───┬───┬─────┬─────┬─────┬───┬───┐
; │ Base 31-12 │ AVL │ G │ 0 │ D │ A │ PCD │ PWT │ U/S │ W │ P │
; └────────────┴─────┴───┴───┴───┴───┴─────┴─────┴─────┴───┴───┘
;                      ▲       ▲   ▲                ▲    ▲   ▲
;                      │       │   │                │    │   └─ 1: The page presents.
;                      │       │   │                │    └─ 1: The page is writable.
;                      │       │   │                └─ 1: The page is at user level.
;                      │       │   │                   0: The page is at supervisor level.
;                      │       │   └─ 1: The page has been accessed.
;                      │       └─ 1: The page is dirty (modified).
;                      └─ 1: The page is global.
; ```
; The page presents.
mem_page_p          equ     1
; The page is read-only.
mem_page_rw_r       equ     0b00
; The page is writable.
mem_page_rw_w       equ     0b10
; The page is at supervisor level.
mem_page_us_s       equ     0b000
; The page is at user level.
mem_page_us_u       equ     0b100

; The size of a page in bytes.
mem_page_size           equ     KB(4)
; The size of a page table entry or a page directory entry in bytes.
mem_page_entry_size     equ     B(4)
; The number of page directory entries in the page directory table.
page_dir_count          equ     mem_page_size / mem_page_entry_size
; The index of the page directory entry (the last one) used to refer to the page directory table itself.
page_dir_self_ref       equ     page_dir_count - 1