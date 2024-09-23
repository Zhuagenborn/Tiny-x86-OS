# Master Boot Record

The *Master Boot Record (MBR)* is the information in the first sector of a hard disk. It identifies how and where the system's operating system is located and a program that loads the rest of the operating system into memory. It will be loaded at physical address `0x7C00` by BIOS.

The following code shows the main functions of `mbr.bin`:

1. Clearing the screen.
2. Loading `loader.bin` from the disk.
3. Jumping to `loader.bin`.

```nasm
; src/boot/mbr.asm

section     mbr     vstart=mbr_base
    ; ...
    call    ClearScreen
    call    ReadLoader
    jmp     loader_code_entry
    ; ...
    times   disk_sector_size - 2 - ($ - $$)  db  0
    db      0x55, 0xAA
```

`mbr.bin` is 512 bytes, ending with `0x55` and `0xAA`. We need to set the start address to `0x7C00` using the `vstart` command. During installation, `mbr.bin` will be written into the first sector of `kernel.img`.