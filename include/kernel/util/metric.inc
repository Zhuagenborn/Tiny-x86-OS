%define     B(n)    (n)
; Convert kilobytes to bytes.
%define     KB(n)   (n * B(1024))
; Convert megabytes to bytes.
%define     MB(n)   (n * KB(1024))
; Convert gigabytes to bytes.
%define     GB(n)   (n * MB(1024))

True    equ     1
False   equ     0