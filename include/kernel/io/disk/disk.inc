; Disk configurations.

%include "kernel/util/metric.inc"

; The size of a sector in bytes.
disk_sector_size                    equ     B(512)
; The maximum number of sectors that can be manipulated per disk access.
; It should be 256, but we set it to 255 to simplify the code.
max_disk_sector_count_per_access    equ     255

; The register for the sector count.
disk_sector_count_port  equ     0x1F2
; The register for the low bits of the start LBA.
disk_lba_low_port       equ     0x1F3
; The register for the middle bits of the start LBA.
disk_lba_mid_port       equ     0x1F4
; The register for the high bits of the start LBA.
disk_lba_high_port      equ     0x1F5

; The device register:
; ```
;   7    6    5    4    3-0
; ┌───┬─────┬───┬─────┬─────┐
; │ 1 │ MOD │ 1 │ DEV │ LBA │
; └───┴─────┴───┴─────┴─────┘
;        ▲         ▲     ▲
;        │         │     └─ The bits 24-27 of the LBA.
;        │         └─ 0: The drive is the master.
;        │            1: The drive is the slave.
;        └─ 0: The addressing mode is Cylinder-Head-Sector (CHS).
;           1: The addressing mode is Logical Block Addressing (LBA).
; ```
disk_device_port        equ     0x1F6
; The disk is in LBA mode.
disk_device_lba_mode    equ     0b0100_0000

; The status register:
; ```
;    7     6    5 4   3   2 1   0
; ┌─────┬──────┬─┬─┬─────┬─┬─┬─────┐
; │ BSY │ DRDY │ │ │ DRQ │ │ │ ERR │
; └─────┴──────┴─┴─┴─────┴─┴─┴─────┘
;    ▲     ▲          ▲         ▲
;    │     │          │         └─ 1: Error occurred.
;    │     │          └─ 1: Data is ready.
;    │     └─ 1: The drive is ready for a command.
;    └─ 1: The drive is busy.
; ```
disk_status_port        equ     0x1F7
; The disk is ready for reading or writing.
disk_status_ready       equ     0b0000_1000
; The disk is busy.
disk_status_busy        equ     0b1000_0000

; The command register.
disk_cmd_port           equ     disk_status_port
; The data register.
disk_data_port          equ     0x1F0

; The identify command.
disk_identify_cmd       equ     0xEC
; The reading command.
disk_read_cmd           equ     0x20
; The writing command.
disk_write_cmd          equ     0x30