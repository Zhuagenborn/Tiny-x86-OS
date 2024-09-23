# Tiny *x86* Operating System in *C++*

![C++](docs/badges/C++.svg)
![NASM](docs/badges/NASM.svg)
![Make](docs/badges/Made-with-Make.svg)
![GitHub Actions](docs/badges/Made-with-GitHub-Actions.svg)
![Linux](docs/badges/Linux.svg)
![License](docs/badges/License-MIT.svg)

## Introduction

This project is a tiny *Intel x86* operating system written in *assembly* and *C++*, developed on *Ubuntu* and *Bochs*.

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

## License

Distributed under the *MIT License*. See `LICENSE` for more information.