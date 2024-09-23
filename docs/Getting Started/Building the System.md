# Building the System

To build the system, we need to clone the project and run the following commands in the project folder:

```bash
# Build the system.
make
# Write built files into a virtual disk.
make install
```

The `make` command only generates binary files. We also need to write them into the virtual disk image `kernel.img` by `make install`.

## Compilation

### *Assembly*

The *assembly* code is built by *NASM*.

```make
# Makefile

ASFLAGS := -f elf \
	-i$(INC_DIR)
```

- `-f elf` generates ELF files

### *C++*

The *C++* code is built by *g++*.

```make
# Makefile

CXXFLAGS := -m32 \
	-std=c++20 \
	-c \
	-I$(INC_DIR) \
	-Wall \
	-O1 \
	-fno-pic \
	-fno-builtin \
	-fno-rtti \
	-fno-exceptions \
	-fno-threadsafe-statics \
	-fno-stack-protector \
	-Wno-missing-field-initializers
```

- `-m32` generates 32-bit code.
- `-c` only compiles code but does not link them.
- `-std=c++20` enables *C++20* features.
- `-fno-pic` generates position-dependent code without a global offset table. Our kernel does not need address relocation or dynamic libraries.
- `-O1` can reduce the stack size for local variables. Otherwise threads may have stack overflow errors.

We also have to add the following options since our kernel does not have *C++* runtime.

- `-fno-builtin`
- `-fno-rtti`
- `-fno-exceptions`
- `-fno-threadsafe-statics`
- `-fno-stack-protector`

## Linking

We use the `ld` command to link *assembly* and *C++* files.

```make
# Makefile

LDFLAGS := -m elf_i386 \
	-Ttext $(CODE_ENTRY) \
	-e main
```

- `-m elf_i386` generates 32-bit ELF files.
- `-e main` uses the `main` function as the entry point.
- `-Ttext $(CODE_ENTRY)` uses `CODE_ENTRY` as the starting address of the `text` segment.

We can get three binary files after linking:

- `mbr.bin` is the master boot record called by BIOS. It loads `loader.bin`.
- `loader.bin` enables memory segmentation, enters protected mode, enables memory paging and loads `kernel.bin`.
- `kernel.bin` is our kernel.

## Installation

The `dd` command can write generated binary files into the virtual system drive `kernel.img`. The following table shows their *Logical Block Addressing (LBA)* ranges. The size of a disk sector is 512 bytes.

|    Module    |  LBAs   |
| :----------: | :-----: |
|  `mbr.bin`   |   `0`   |
| `loader.bin` | `1`-`5` |
| `kernel.bin` |  `6`-   |

```make
# Makefile

dd if=$(BUILD_DIR)/boot/mbr.bin of=$(DISK) bs=512 count=1 conv=notrunc
dd if=$(BUILD_DIR)/boot/loader.bin of=$(DISK) seek=1 bs=512 count=$(LOADER_SECTOR_COUNT) conv=notrunc
dd if=$(BUILD_DIR)/kernel.bin of=$(DISK) bs=512 seek=$(KRNL_START_SECTOR) count=$(KRNL_SECTOR_COUNT) conv=notrunc
```

This table shows all `make` targets in `Makefile`.

|  Target   |                               Usage                               |
| :-------: | :---------------------------------------------------------------: |
|  `build`  |                  Building all modules by default                  |
|  `boot`   |                Building `mbr.bin` and `loader.bin`                |
| `kernel`  |                       Building `kernel.bin`                       |
|  `user`   |                       Building user modules                       |
| `install` | Installing all modules into the virtual system drive `kernel.img` |
|  `clean`  |                      Cleaning up built files                      |