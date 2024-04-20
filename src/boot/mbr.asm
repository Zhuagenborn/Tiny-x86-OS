; The Master Boot Record (MBR)
; It loads the kernel loader.

%include "boot/boot.inc"
%include "kernel/krnl.inc"
%include "kernel/io/video/print.inc"
%include "kernel/io/disk/disk.inc"

; The BIOS will load the master boot record to `0x7C00`.
section     mbr     vstart=mbr_base
    mov     ax, cs
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     fs, ax
    mov     gs, ax
    mov     sp, mbr_stack_top

    call    ClearScreen

    call    ReadLoader
    jmp     loader_code_entry

; Read the loader from the disk.
ReadLoader:
%if loader_sector_count > max_disk_sector_count_per_access
    %error "The loader is too large"
%endif
    mov     ax, loader_start_sector
    mov     bx, loader_base
    mov     cx, loader_sector_count
    call    ReadDisk
    ret

; Read data from the disk to memory in 16-bit mode.
; Input:
; `AX` = A start sector where data will be read from.
; `BX` = A physical address where data will be written to.
; `CX` = The number of sectors to be read.
ReadDisk:
    mov     si, ax
    mov     di, cx
    cmp     cx, 0
    je      .end

    ; Set the sector count.
    mov     dx, disk_sector_count_port
    mov     al, cl
    out     dx, al

    mov     ax, si

    ; Set the start sector.
    ; The bits `0`-`23` of the sector should be saved to disk ports `0x1F3`, `0x1F4`, `0x1F5`.
    ; Set the bits `0`-`7`.
    mov     dx, disk_lba_low_port
    out     dx, al
    ; Set the bits `8`-`15`.
    mov     cl, 8
    shr     ax, cl
    mov     dx, disk_lba_mid_port
    out     dx, al
    ; Set the bits `16`-`23`.
    shr     ax, cl
    mov     dx, disk_lba_high_port
    out     dx, al
    ; The bits `24`-`27` of the sector should be saved to the bits `0`-`3` of the disk port `0x1F6`.
    shr     ax, cl
    and     al, 0xF
    ; Set the bits `4`-`7` of the port `0x1F6` to `0b1110`.
    or      al, 0b1010_0000 | disk_device_lba_mode
    mov     dx, disk_device_port
    out     dx, al

    ; Write a reading command to the disk port `0x1F7`.
    mov     dx, disk_cmd_port
    mov     al, disk_read_cmd
    out     dx, al

    ; Check disk status using the same port `0x1F7`.
.not_ready:
    nop     ; Sleep
    in      al, dx
    and     al, disk_status_ready | disk_status_busy
    cmp     al, disk_status_ready
    jnz     .not_ready

    ; Read words from the disk port `0x1F0`.
    ; `DI` is the number of sectors to be read. Each sector has 512 bytes.
    ; Each reading reads 2 bytes, so `(512 * DI) / 2` readings are needed.
    mov     ax, di
    mov     dx, 256
    mul     dx
    ; The high bits of the multiplication result in `DX` are ignored since the loader size is small.
    mov     cx, ax

    mov     dx, disk_data_port
    ; The offset ranges from `0x0000` to `0xFFFF`, so the maximum loader size is 64 KB.
    mov     di, bx
    cld
    rep     insw
.end:
    ret

ClearScreen:
    mov     ax, 0x600
    mov     bx, 0x700
    xor     cx, cx
    mov     dx, ((text_screen_height - 1) << 8) + (text_screen_width - 1)
    int     0x10
    ret

    ; The master boot record has 512 bytes and ends with `0x55`, `0xAA`.
    times   disk_sector_size - 2 - ($ - $$)  db  0
    db      0x55, 0xAA