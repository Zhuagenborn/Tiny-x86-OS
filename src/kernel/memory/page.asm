%include "kernel/util/metric.inc"

[bits 32]
section     .text
global      DisableTlbEntry
; Invalidate a Translation Lookaside Buffer (TLB) entry.
; When the CPU needs to translate a virtual address into a physical address, the TLB is consulted first.
; If paging structures are modified, the TLB is not transparently informed of changes.
; Therefore the TLB has to be flushed.
DisableTlbEntry:
    %push   disable_tle_entry
    %stacksize  flat
    %arg    vr_addr:dword
        enter   B(0), 0
        mov     eax, [vr_addr]
        invlpg  [eax]
        leave
        ret
    %pop