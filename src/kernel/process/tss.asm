%include "kernel/util/metric.inc"

[bits 32]
section     .text
global      SetTaskReg
; Set the task register.
SetTaskReg:
    %push   set_task_reg
    %stacksize  flat
    %arg    sel:word
        enter   B(0), 0
        ltr     [sel]
        leave
        ret
    %pop