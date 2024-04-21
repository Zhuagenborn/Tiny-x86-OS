; The kernel loader
; It enables memory segmentation, enters protected mode, enables memory paging and loads the kernel.

%include "boot/boot.inc"
%include "kernel/process/elf.inc"
%include "kernel/util/metric.inc"
%include "kernel/krnl.inc"
%include "kernel/selector/sel.inc"
%include "kernel/descriptor/desc.inc"
%include "kernel/memory/page.inc"
%include "kernel/io/disk/disk.inc"

; The maximum number of global descriptors.
gdt_count       equ     60

; The control register of the A20 address line.
a20_ctrl_port   equ     0x92

; Protected mode is enable.
cr0_pe          equ     1
; Memory paging is enable.
cr0_pg          equ     0x80000000

; The address range descriptor.
struc       AddrRangeDesc
    .base_addr_low:     resd    1
    .base_addr_high:    resd    1
    .len_low:           resd    1
    .len_high:          resd    1
    .type:              resd    1
endstruc

section     loader  vstart=loader_base
; The Global Descriptor Table (GDT)
; -------------------------------------------------------------------
gdt_base:
; The first descriptor is unavailable.
; It is used to raise an exception if a selector is not initialized.
; Also, when the current privilege level changes from high to low,
; segment registers may still refer to a high-privileged segment.
; In this case, these segment registers will be set to `0` for safety.
    dd      0, 0
; Code segments range from `0x00000000` to `0xFFFFFFFF`.
code_desc:
    dd      0x0000FFFF, desc_code_high_32_bits
; Data segments range from `0x00000000` to `0xFFFFFFFF`.
data_desc:
    dd      0x0000FFFF, desc_data_high_32_bits
; The VGA text buffer ranges from `0x000B8000` to `0x00007FFF`.
video_desc:
    dd      0x80000007, desc_video_high_32_bits

    times       gdt_count - 4       dq      0
    ; The size of the global descriptor table in bytes.
    gdt_size    equ     $ - gdt_base
    gdt_limit   equ     gdt_size - 1

%if $ - gdt_base != gdt_count * desc_size
    %error "The size of the global descriptor table does not meet the predefinition"
%endif
    ; -------------------------------------------------------------------

    ; The total memory size in bytes.
    total_mem_size    dd      0

    gdt_reg:
        istruc  DescTabReg
            at DescTabReg.limit,     dw      gdt_limit
            at DescTabReg.base,      dd      gdt_base
        iend

    ard:
        istruc  AddrRangeDesc
            at AddrRangeDesc.base_addr_low,    dd      0
            at AddrRangeDesc.base_addr_high,   dd      0
            at AddrRangeDesc.len_low,          dd      0
            at AddrRangeDesc.len_high,         dd      0
            at AddrRangeDesc.type,             dd      0
        iend

    align       loader_data_size

%if $ - $$ != loader_data_size
    %error "The data size does not meet the predefinition"
%endif

start:
%if $ - $$ != loader_code_entry - loader_base
    %error "The code address does not meet the predefinition"
%endif

    call    InitTotalMemSize

    ; Enable memory segmentation and protected mode.

    ; Enable the A20 line.
    in      al, a20_ctrl_port
    or      al, 0b0010
    out     a20_ctrl_port, al

    ; Load the global descriptor table.
    lgdt    [gdt_reg]

    ; Set the `PE` bit of `CR0`.
    mov     eax, cr0
    or      eax, cr0_pe
    mov     cr0, eax

    ; Check the global descriptor table in Bochs.
    ; ```console
    ; -----------------------------------------------------------------------------------------------------------
    ; <bochs:> info gdt 1 3
    ; Global Descriptor Table (base=0xC0000900, limit=2303):
    ; GDT[0x01]=Code segment, base=0x00000000, limit=0xFFFFFFFF, Execute-Only, Non-Conforming, Accessed, 32-bit
    ; GDT[0x02]=Data segment, base=0x00000000, limit=0xFFFFFFFF, Read/Write, Accessed
    ; GDT[0x03]=Data segment, base=0x000B8000, limit=0x00007FFF, Read/Write, Accessed
    ; -----------------------------------------------------------------------------------------------------------
    ; ```

    ; Flush the segment descriptor cache and CPU pipelines, then enter protected mode.
    ; Because subsequent instructions have been incorrectly decoded in 16-bit mode in pipelines.
    jmp     sel_code:protected_mode

; Get the total memory size in bytes.
InitTotalMemSize:
    call    GetTotalMemSizeByInt15AxE820
    test    eax, eax
    jz      .try_int_15_ax_e801
    mov     [total_mem_size], ecx
    ret

.try_int_15_ax_e801:
    call    GetTotalMemSizeByInt15AxE801
    test    eax, eax
    jz      .try_int_15_ah_88
    mov     [total_mem_size], ecx
    ret

.try_int_15_ah_88:
    call    GetTotalMemSizeByInt15Ah88
    test    eax, eax
    jz      .error
    mov     [total_mem_size], ecx
.error:
    ret

; Get the total memory size in bytes by `INT 0x15`, `AH 0x88`.
; Output:
; `EAX` = Successful or not.
; `ECX` = The memory size.
GetTotalMemSizeByInt15Ah88:
    xor     ecx, ecx
    mov     ah, 0x88
    int     0x15
    jc      .error
    and     eax, 0x0000FFFF
    ; Convert the unit of memory size from KB to byte.
    mov     cx, KB(1)
    mul     cx
    shl     edx, 16
    or      edx, eax
    ; The `0x88` function does not count the first 1 MB memory.
    add     edx, MB(1)
    mov     ecx, edx
    mov     eax, True
    ret
.error:
    xor     eax, eax
    ret

; Get the total memory size in bytes by `INT 0x15`, `AX 0xE820`.
; Output:
; `EAX` = Successful or not.
; `ECX` = The memory size.
GetTotalMemSizeByInt15AxE820:
    xor     ecx, ecx
    xor     ebx, ebx
    mov     di, ard

    ; Check each memory segment and find the maximum of `(AddrRangeDesc.base_addr_low + AddrRangeDesc.len_low)` of segments.
.read_seg:
    push    ecx
    mov     edx, 0x534D4150 ; "SMAP"
    mov     eax, 0x0000E820
    mov     ecx, AddrRangeDesc_size
    int     0x15
    jc      .error
    pop     ecx
    ; `ECX` is the maximum of `(AddrRangeDesc.base_addr_low + AddrRangeDesc.len_low)` of segments.
    mov     eax, [ard + AddrRangeDesc.base_addr_low]
    add     eax, [ard + AddrRangeDesc.len_low]
    cmp     eax, ecx
    jle     .not_change_max
    mov     ecx, eax
.not_change_max:
    test    ebx, ebx
    jnz     .read_seg
    mov     eax, True
    ret
.error:
    pop     ecx
    xor     eax, eax
    ret

; Get the total memory size in bytes by `INT 0x15`, `AX 0xE801`.
; Output:
; `EAX` = Successful or not.
; `ECX` = The memory size.
GetTotalMemSizeByInt15AxE801:
    xor     ecx, ecx
    xor     eax, eax
    mov     ax, 0xE801
    int     0x15
    jc      .error

    ; Count the low 15 MB memory.
    ; Convert the unit of memory size from KB to byte.
    mov     cx, KB(1)
    mul     cx
    shl     edx, 16
    and     eax, 0x0000FFFF
    or      edx, eax

    ; Count the 1 MB memory hole between 15 MB and 16 MB.
    add     edx, MB(1)
    mov     esi, edx

    ; Count the memory above 16 MB.
    xor     eax, eax
    mov     ax, bx
    ; Convert the unit of memory size from 64 KB to byte.
    mov     ecx, KB(64)
    mul     ecx
    ; `EAX` is enough since the `0xE801` function can only detect up to 4 GB memory.
    add     esi, eax
    mov     ecx, esi
    mov     eax, True
    ret
 .error:
    xor     eax, eax
    ret

[bits 32]
protected_mode:
    mov     ax, sel_data
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     ax, sel_video
    mov     gs, ax
    mov     esp, loader_stack_top

    call    EnableMemPaging
    call    LoadKernel
    mov     esp, krnl_stack_top
    ; Jump to the kernel.
    call    eax
.end:
    jmp     .end

EnableMemPaging:
    call    SetupPageDir

    sgdt    [gdt_reg]
    mov     ebx, [gdt_reg + DescTabReg.base]

    ; Add `0xC0000000` to the address of the VGA text buffer.
    or      dword [ebx + desc_size * 3 + B(4)], krnl_base
    ; Add `0xC0000000` to the address of the global descriptor table.
    add     dword [gdt_reg + DescTabReg.base], krnl_base
    ; Add `0xC0000000` to the stack address.
    add     esp, krnl_base

    ; Set `CR3` to the address of the page directory table.
    mov     eax, page_dir_base
    mov     cr3, eax

    ; Set the `PG` bit of `CR0`.
    mov     eax, cr0
    or      eax, cr0_pg
    mov     cr0, eax

    ; Reload the global descriptor table.
    lgdt    [gdt_reg]

    ; Flush CPU pipelines. Theoretically it is not needed.
    jmp     sel_code:paging_enabled
paging_enabled:
    ret

; Initialize the page directory table.
; ```
;     ┌─────────────────────────┐
;     │           ...           │
;     ├─────────────────────────┤
;     │     (1) Page Table      │ ◄─────────┐
;     ├─────────────────────────┤           │
; ┌── │     (0) Page Table      │ ◄──┐      │
; │   ├─────────────────────────┤    │      │
; │   │ (1023) Directory Entry  │ ───│───┐  │
; │   ├─────────────────────────┤    │   │  │
; │   │           ...           │    │   │  │
; │   ├─────────────────────────┤    │   │  │
; │   │ (0x301) Directory Entry │ ───│───│──┘
; │   ├─────────────────────────┤    │   │
; │   │ (0x300) Directory Entry │ ───┤   │
; │   ├─────────────────────────┤    │   │
; │   │           ...           │    │   │
; │   ├─────────────────────────┤    │   │
; │   │   (0) Directory Entry   │ ───┘ ◄─┘
; │   ├─────────────────────────┤
; └─► │         Kernel          │
;     └─────────────────────────┘
; ```
SetupPageDir:
    call    ClearPageDir

    ; Initialize the page directory table.

    mov     eax, page_dir_base
    add     eax, mem_page_size
    ; `EBX` is the physical address of the first page table.
    ; It will map the first 4 MB physical memory from `0x00000000` to `0x003FFFFF`.
    mov     ebx, eax
    or      eax, mem_page_us_u | mem_page_rw_w | mem_page_p

    ; The first page directory entry indexed `0` points to the first page table.
    ; Because currently the loader is loaded to the first 1 MB physical memory within `0x00000000`-`0x003FFFFF`.
    ; To make sure it works properly, the linear address under memory segmentation and the virtual address under memory paging must be mapped to the same physical address.
    mov     [page_dir_base + mem_page_entry_size * 0], eax

    ; The page directory entry indexed `0x300` also points to the first page table.
    ; It is prepared for the kernel, which will be loaded to the first 1 MB physical memory.
    ; Because in virtual memory, the user space ranges from `0x00000000` to `0xBFFFFFFF`, 3 GB totally.
    ; Kernel space ranges from `0xC0000000` to `0xFFFFFFFF`, 1 GB totally.
    ; The page directory entry index for `0xC0000000` is `0xC0000000 & 0xFFC00000 = 0x300`.
    mov     [page_dir_base + mem_page_entry_size * krnl_page_dir_start], eax

    ; The last page directory entry indexed `1023` points to the page directory table itself for access.
    sub     eax, mem_page_size
    mov     [page_dir_base + mem_page_entry_size * page_dir_self_ref], eax

    ; Initialize the first page table.
    ; It maps virtual memory `0x00000000`-`0x003FFFFF` and `0xC0000000`-`0xC03FFFFF` to physical memory `0x00000000`-`0x003FFFFF`.
    mov     ecx, krnl_size / mem_page_size
    xor     esi, esi
    xor     edx, edx
    or      edx, mem_page_us_u | mem_page_rw_w | mem_page_p
.init_page_tab:
    mov     [ebx + mem_page_entry_size * esi], edx
    add     edx, mem_page_size
    inc     esi
    loop    .init_page_tab

    ; Initialize other page directory entries for the kernel.
    ; When creating a page directory table for a new process, they will be copied to the same index.
    ; So all processes can share the same kernel space.
    mov     eax, page_dir_base
    ; `EAX` is the physical address of the second page table.
    add     eax, mem_page_size * 2
    or      eax, mem_page_us_u | mem_page_rw_w | mem_page_p
    mov     ebx, page_dir_base

    ; Initialize the page directory entries indexed from `0x301` to `1022`.
    mov     ecx, krnl_page_dir_count - 1
    mov     esi, krnl_page_dir_start + 1
.init_page_dir:
    mov     [ebx + mem_page_entry_size * esi], eax
    inc     esi
    add     eax, mem_page_size
    loop    .init_page_dir

    ; Check address mapping in Bochs.
    ; ```console
    ; ------------------------------------------------------------------------------------------------------------------------------
    ; <bochs:> info tab
    ; CR3: 0x000000100000
    ; 0x00000000-0x000FFFFF -> 0x000000000000-0x0000000FFFFF
    ; 0xC0000000-0xC00FFFFF -> 0x000000000000-0x0000000FFFFF
    ; 0xFFC00000-0xFFC00FFF -> 0x000000101000-0x000000101FFF
    ; 0xFFFFF000-0xFFFFFFFF -> 0x000000100000-0x000000100FFF
    ; ------------------------------------------------------------------------------------------------------------------------------
    ; ```
    ;
    ; `0x00000000-0x000FFFFF -> 0x000000000000-0x0000000FFFFF`:
    ; The first page directory entry indexed `0` is pointing to the first page table.
    ;
    ; `0xC0000000-0xC00FFFFF -> 0x000000000000-0x0000000FFFFF`:
    ; The page directory entry indexed `0x300` is also pointing to the first page table.
    ;
    ; `0xFFC00000-0xFFC00FFF -> 0x000000101000-0x000000101FFF`:
    ; 1. The high 10 bits of `0xFFC00000` are `1023`.
    ;    The last page directory entry indexed `1023` is pointing to the page directory table itself at `0x00100000`.
    ;    The page directory table is treated as a page table because it is accessed from a page directory entry.
    ; 2. The middle 10 bits of `0xFFC00000` are `0`.
    ;    The first page directory entry indexed `0`, which is pointing to the first page table at `0x00101000`, is treated as a page entry.
    ;    So the virtual address `0xFFC00000` is mapped to the physical address `0x000000101000`.
    ;    `0xFFC00000` can be used to access page tables.
    ; 3. By setting the value of the first 10 bits, we can access different page tables.
    ;
    ; `0xFFFFF000-0xFFFFFFFF -> 0x000000100000-0x000000100FFF`:
    ; 1. The high 10 bits of `0xFFFFF000` are `1023`.
    ;    The last page directory entry indexed `1023` is pointing to the page directory table itself at `0x00100000`.
    ;    The page directory table is treated as a page table because it is accessed from a page directory entry.
    ; 2. The middle 10 bits of `0xFFFFF000` are `1023`.
    ;    The last page directory entry indexed `1023`, which is pointing to the page directory table itself at `0x00100000`, is treated as a page entry.
    ;    So the virtual address `0xFFFFF000` is mapped to the physical address `0x000000100000`.
    ;    `0xFFFFF000` can be used to access the page directory table.
    ; 3. By setting the value of the first 10 bits, we can access different page directory entries.
    ret

ClearPageDir:
    mov     ecx, mem_page_size
    xor     esi, esi
.clear:
    mov     byte [page_dir_base + esi], 0
    inc     esi
    loop    .clear
    ret

; Copy memory from a location to another.
MemCopy:
    %push   mem_copy
    %stacksize  flat
    %arg    dest:dword, src:dword, size:dword
        enter   B(0), 0
        cld
        push    ecx
        mov     edi, [dest]
        mov     esi, [src]
        mov     ecx, [size]
        rep     movsb
        pop     ecx
        leave
        ret
    %pop

; Load the kernel image to memory.
; Output:
; `EAX` = The entry point.
LoadKernel:
    call    ReadKernel

    xor     eax, eax
    xor     ebx, ebx
    xor     ecx, ecx
    xor     edx, edx

    ; The kernel is built as an ELF file.
    mov     eax, [raw_krnl_base + ElfFileHeader.e_entry]
    push    eax
    mov     dx, [raw_krnl_base + ElfFileHeader.e_phentsize]
    mov     ebx, [raw_krnl_base + ElfFileHeader.e_phoff]
    add     ebx, raw_krnl_base

    ; Copy each segment to its virtual address.
    mov     cx, [raw_krnl_base + ElfFileHeader.e_phnum]
.load_seg:
    cmp     byte [ebx + ElfSectHeader.p_type], PT_NULL
    je      .next_seg
    push    dword [ebx + ElfSectHeader.p_filesz]
    mov     eax, [ebx + ElfSectHeader.p_offset]
    add     eax, raw_krnl_base
    push    eax
    push    dword [ebx + ElfSectHeader.p_vaddr]
    call    MemCopy
    add     esp, B(12)
.next_seg:
    add     ebx, edx
    loop    .load_seg
    pop     eax
    ret

; Read the kernel from the disk.
ReadKernel:
%if krnl_sector_count > 1500
    %error "The kernel is too large"
%endif
    mov     eax, krnl_start_sector
    mov     ebx, raw_krnl_base
    mov     ecx, krnl_sector_count

.read:
    ; Read up to 255 sectors at a time.
    push    ecx
    cmp     ecx, max_disk_sector_count_per_access
    jb      .not_change_sector
    mov     ecx, max_disk_sector_count_per_access
.not_change_sector:
    push    eax
    push    ebx
    call    ReadDisk
    pop     ebx
    pop     eax
    pop     ecx
    cmp     ecx, max_disk_sector_count_per_access
    jb      .end
    sub     ecx, max_disk_sector_count_per_access
    add     eax, max_disk_sector_count_per_access
    add     ebx, max_disk_sector_count_per_access * disk_sector_size
    jmp     .read
.end:
    ret

; Read data from the disk to memory in 32-bit mode.
; Input:
; `EAX` = A start sector where data will be read from.
; `EBX` = A physical address where data will be written to.
; `CX` = The number of sectors to be read.
ReadDisk:
    mov     esi, eax
    mov     di, cx
    cmp     cx, 0
    je      .end

    ; Set the sector count.
    mov     dx, disk_sector_count_port
    mov     al, cl
    out     dx, al

    mov     eax, esi

    ; Set the start sector.
    ; The bits `0`-`23` of the sector should be saved to disk ports `0x1F3`, `0x1F4`, `0x1F5`.
    ; Set the bits `0`-`7`.
    mov     dx, disk_lba_low_port
    out     dx, al
    ; Set the bits `8`-`15`.
    mov     cl, 8
    shr     eax, cl
    mov     dx, disk_lba_mid_port
    out     dx, al
    ; Set the bits `16`-`23`.
    shr     eax, cl
    mov     dx, disk_lba_high_port
    out     dx, al
    ; The bits `24`-`27` of the sector should be saved to the bits `0`-`3` of the disk port `0x1F6`.
    shr     eax, cl
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
    ; The high bits of the multiplication result in `EDX` are ignored.
    xor     ecx, ecx
    mov     cx, ax

    mov     dx, disk_data_port
    mov     edi, ebx
    cld
    rep     insw
.end:
    ret