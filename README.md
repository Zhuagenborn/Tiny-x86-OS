# Tiny *x86* Operating System in *C++*

![C++](docs/badges/C++.svg)
![NASM](docs/badges/NASM.svg)
![Make](docs/badges/Made-with-Make.svg)
![GitHub Actions](docs/badges/Made-with-GitHub-Actions.svg)
![Linux](docs/badges/Linux.svg)
![License](docs/badges/License-MIT.svg)

## Introduction

A simple *Intel x86* operating system written in *assembly* and *C++*, developed on *Ubuntu* and *Bochs*.

- Boot
  - The master boot record for system startup.
- Memory
  - Memory segmentation and paging.
  - Virtual memory mapping based on bitmaps.
  - Heap management (`std::malloc` and `std::free`) based on memory arenas.
- Interrupts
  - Interrupt control based on *Intel 8259A*.
  - Timer interrupts based on *Intel 8253*.
- Threads
  - Thread scheduling based on timer interrupts.
  - Semaphores and locks based on interrupts.
- Processes
  - User processes based on *Intel x86* task state segments.
  - Fork.
- Graphic
  - Character printing in VGA text mode.
- Keyboard
  - Keyboard control based on *Intel 8042*.
  - The circular keyboard input buffer.
- Disks
  - IDE channel and disk control.
  - Disk partition scanning.
- File System
  - File and directory management based on index nodes.
- System Calls
  - Privilege switching and system calls based on interrupts.
- *C/C++*
  - Basic *C/C++* standard libraries.

## Contents

### Getting Started

1. [Development Environment](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Getting%20Started/Development%20Environment.md)
2. [Building the System](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Getting%20Started/Building%20the%20System.md)

### Boot

3. [Master Boot Record](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Boot/Master%20Boot%20Record.md)
4. [Loader](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Boot/Loader.md)

### Kernel

5. [Interrupts](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/Interrupts.md)
6. [System Calls](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/System%20Calls.md)
7. [Threads](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/Threads.md)
8. [Memory](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/Memory.md)
9. [File System](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/File%20System.md)
10. [User Processes](https://github.com/Zhuagenborn/Tiny-x86-OS/blob/main/docs/Kernel/User%20Processes.md)

## References

- [*《操作系统真象还原》郑钢*](https://github.com/yifengyou/os-elephant)

## Structure

```console
.
├── CITATION.cff
├── Debugging.md
├── LICENSE
├── Makefile
├── README.md
├── docs
│   ├── Boot
│   │   ├── Images
│   │   │   └── loader
│   │   │       ├── memory-paging.drawio
│   │   │       ├── memory-paging.svg
│   │   │       ├── page-directory-table.drawio
│   │   │       └── page-directory-table.svg
│   │   ├── Loader.md
│   │   └── Master Boot Record.md
│   ├── Getting Started
│   │   ├── Building the System.md
│   │   └── Development Environment.md
│   ├── Kernel
│   │   ├── File System.md
│   │   ├── Images
│   │   │   ├── file-system
│   │   │   │   ├── directory-entries.drawio
│   │   │   │   ├── directory-entries.svg
│   │   │   │   ├── index-node.drawio
│   │   │   │   └── index-node.svg
│   │   │   ├── memory
│   │   │   │   ├── memory-heap.drawio
│   │   │   │   ├── memory-heap.svg
│   │   │   │   ├── memory-pools.drawio
│   │   │   │   └── memory-pools.svg
│   │   │   └── threads
│   │   │       ├── thread-block.drawio
│   │   │       ├── thread-block.svg
│   │   │       ├── thread-lists.drawio
│   │   │       ├── thread-lists.svg
│   │   │       ├── thread-switching.drawio
│   │   │       └── thread-switching.svg
│   │   ├── Interrupts.md
│   │   ├── Memory.md
│   │   ├── System Calls.md
│   │   ├── Threads.md
│   │   └── User Processes.md
│   └── badges
│       ├── C++.svg
│       ├── License-MIT.svg
│       ├── Linux.svg
│       ├── Made-with-GitHub-Actions.svg
│       ├── Made-with-Make.svg
│       └── NASM.svg
├── include
│   ├── boot
│   │   └── boot.inc
│   ├── kernel
│   │   ├── debug
│   │   │   └── assert.h
│   │   ├── descriptor
│   │   │   ├── desc.h
│   │   │   ├── desc.inc
│   │   │   └── gdt
│   │   │       ├── idx.h
│   │   │       └── tab.h
│   │   ├── interrupt
│   │   │   ├── intr.h
│   │   │   └── pic.h
│   │   ├── io
│   │   │   ├── disk
│   │   │   │   ├── disk.h
│   │   │   │   ├── disk.inc
│   │   │   │   ├── file
│   │   │   │   │   ├── dir.h
│   │   │   │   │   ├── file.h
│   │   │   │   │   ├── inode.h
│   │   │   │   │   └── super_block.h
│   │   │   │   └── ide.h
│   │   │   ├── file
│   │   │   │   ├── dir.h
│   │   │   │   ├── file.h
│   │   │   │   └── path.h
│   │   │   ├── io.h
│   │   │   ├── keyboard.h
│   │   │   ├── timer.h
│   │   │   └── video
│   │   │       ├── console.h
│   │   │       ├── print.h
│   │   │       └── print.inc
│   │   ├── krnl.h
│   │   ├── krnl.inc
│   │   ├── memory
│   │   │   ├── page.h
│   │   │   ├── page.inc
│   │   │   └── pool.h
│   │   ├── process
│   │   │   ├── elf.inc
│   │   │   ├── proc.h
│   │   │   └── tss.h
│   │   ├── selector
│   │   │   ├── sel.h
│   │   │   └── sel.inc
│   │   ├── stl
│   │   │   ├── algorithm.h
│   │   │   ├── array.h
│   │   │   ├── cerron.h
│   │   │   ├── cmath.h
│   │   │   ├── cstddef.h
│   │   │   ├── cstdint.h
│   │   │   ├── cstdlib.h
│   │   │   ├── cstring.h
│   │   │   ├── iterator.h
│   │   │   ├── mutex.h
│   │   │   ├── semaphore.h
│   │   │   ├── source_location.h
│   │   │   ├── span.h
│   │   │   ├── string_view.h
│   │   │   ├── type_traits.h
│   │   │   └── utility.h
│   │   ├── syscall
│   │   │   └── call.h
│   │   ├── thread
│   │   │   ├── sync.h
│   │   │   └── thd.h
│   │   └── util
│   │       ├── bit.h
│   │       ├── bitmap.h
│   │       ├── block_queue.h
│   │       ├── format.h
│   │       ├── metric.h
│   │       ├── metric.inc
│   │       └── tag_list.h
│   └── user
│       ├── io
│       │   ├── file
│       │   │   ├── dir.h
│       │   │   └── file.h
│       │   └── video
│       │       └── console.h
│       ├── memory
│       │   └── pool.h
│       ├── process
│       │   └── proc.h
│       ├── stl
│       │   └── cstdint.h
│       └── syscall
│           └── call.h
└── src
    ├── boot
    │   ├── loader.asm
    │   └── mbr.asm
    ├── kernel
    │   ├── debug
    │   │   └── assert.cpp
    │   ├── descriptor
    │   │   ├── desc.asm
    │   │   └── gdt
    │   │       └── tab.cpp
    │   ├── interrupt
    │   │   ├── intr.asm
    │   │   ├── intr.cpp
    │   │   └── pic.cpp
    │   ├── io
    │   │   ├── disk
    │   │   │   ├── disk.cpp
    │   │   │   ├── file
    │   │   │   │   ├── dir.cpp
    │   │   │   │   ├── file.cpp
    │   │   │   │   ├── inode.cpp
    │   │   │   │   └── super_block.cpp
    │   │   │   ├── ide.cpp
    │   │   │   └── part.cpp
    │   │   ├── file
    │   │   │   ├── dir.cpp
    │   │   │   ├── file.cpp
    │   │   │   └── path.cpp
    │   │   ├── io.asm
    │   │   ├── io.cpp
    │   │   ├── keyboard.cpp
    │   │   ├── timer.cpp
    │   │   └── video
    │   │       ├── console.cpp
    │   │       ├── print.asm
    │   │       └── print.cpp
    │   ├── krnl.cpp
    │   ├── main.cpp
    │   ├── memory
    │   │   ├── page.asm
    │   │   ├── page.cpp
    │   │   └── pool.cpp
    │   ├── process
    │   │   ├── proc.cpp
    │   │   ├── tss.asm
    │   │   └── tss.cpp
    │   ├── stl
    │   │   ├── cstring.cpp
    │   │   ├── mutex.cpp
    │   │   └── semaphore.cpp
    │   ├── syscall
    │   │   ├── call.asm
    │   │   └── call.cpp
    │   ├── thread
    │   │   ├── sync.cpp
    │   │   ├── thd.asm
    │   │   └── thd.cpp
    │   └── util
    │       ├── bitmap.cpp
    │       ├── format.cpp
    │       └── tag_list.cpp
    └── user
        ├── io
        │   ├── file
        │   │   ├── dir.cpp
        │   │   └── file.cpp
        │   └── video
        │       └── console.cpp
        ├── memory
        │   └── pool.cpp
        └── process
            └── proc.cpp
```

## License

Distributed under the *MIT License*. See `LICENSE` for more information.