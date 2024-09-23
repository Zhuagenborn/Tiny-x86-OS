# Development Environment

The development environment is based on *Ubuntu 22.04* with the following tools:

- [*Bochs 2.7*](https://bochs.sourceforge.io) is a cross-platform *x86* emulator. Our kernel will be running on a *Bochs* virtual machine.
- [*Make 4.3*](https://www.gnu.org/software/make) is an automation tool that controls the generation of executable files.
- [*NASM 2.15.05*](https://www.nasm.us) is a *x86* asssembler which will be used to compile *assembly* code (`.inc`, `.asm`).
- [*g++ 11.4.0*](https://gcc.gnu.org) is a *C++* compiler which will be used to compile *C++* code (`.h`, `.cpp`).

## *Bochs*

### Building

We can download *Bochs* by:

```bash
wget https://sourceforge.net/projects/bochs/files/bochs/2.7/bochs-2.7.tar.gz
tar -zxvf bochs-2.7.tar.gz
```

`<bochs-path>` should be replaced with the *Bochs* installation path.

```console
bochs-2.7$ ./configure --prefix=<bochs-path> --enable-debugger --enable-iodebug --enable-x86-debugger --with-x --with-x11
bochs-2.7$ make
bochs-2.7$ make install
```

A possible error is that GUI libraries were not found.

```console
ERROR: X windows gui was selected, but X windows libraries were not found.
```

It can be fixed by installing `xorg-dev`.

```bash
apt install xorg-dev
```

### Running

First, we need to create a master disk `kernel.img` for the system drive using `bximage`.

```console
bochs/bin$ ./bximage -func=create -hd=60 -imgmode=flat -q kernel.img
```

When it is done, `bximage` shows:

```console
The following line should appear in your bochsrc:
  ata0-master: type=disk, path="kernel.img", mode=flat
```

Second, we need to create a configuration file `bochs/bin/bochsrc.disk`. `<bochs-path>` should be replaced with the *Bochs* installation path.

```console
# The memory usage, 32 MB
megs: 32

# The BIOS and VGA BIOS of the host machine
romimage: file=<bochs-path>/share/bochs/BIOS-bochs-latest
vgaromimage: file=<bochs-path>/share/bochs/VGABIOS-lgpl-latest

# Boot from a disk.
boot: disk

# Logging
log: bochs.log

# Disable the mouse and enable the keyboard.
mouse: enabled=0
keyboard: keymap=<bochs-path>/share/bochs/keymaps/x11-pc-us.map

# Disk
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
# ------------------- bximage output -------------------
ata0-master: type=disk, path="kernel.img", mode=flat
# ------------------------------------------------------

# Uncomment the following to support GDB, which can connect to the port 1234 and debug.
# gdbstub: enabled=1, port=1234, text_base=0, data_base=0, bss_base=0
```

Now we can start *Bochs*.

```console
bochs/bin$ ./bochs -f bochsrc.disk
```

Select `6. Begin simulation` and enter <kbd>c</kbd> to run the emulator.

```console
1. Restore factory default configuration
2. Read options from...
3. Edit options
4. Save options to...
5. Restore the Bochs state from...
6. Begin simulation
7. Quit now

Please choose one: [6] 6
00000000000i[  ] installing x module as the Bochs GUI
00000000000i[  ] using log file bochs.log
Next at t=0
(0) [0x0000fffffff0] f000:fff0 (unk. ctxt): jmp far f000:e05b  ; ea5be000f0
<bochs:1> c
```

### Formatting Call Logs

*Bochs* can generate call logs with the debugger command `show call`, for example:

```console
00016253385: call 0008:c0003dba (0xc0003dba) (phy: 0x000000003dba) unk. ctxt
00016253389: call 0008:c0001566 (0xc0001566) (phy: 0x000000001566) unk. ctxt
00016253399: call 0008:c0003dec (0xc0003dec) (phy: 0x000000003dec) unk. ctxt
```

It only shows function addresses. While the *PowerShell* script [*Format-BochsCallLog*](https://github.com/Zhuagenborn/Bochs-Call-Log-Formatter) can add the corresponding function name for each call with the `nm` command and a provided module. The formatted logs will be:

```console
00016253385: call 0008:c0003dba (0xc0003dba) (phy: 0x000000003dba) unk. ctxt [_function1]
00016253389: call 0008:c0001566 (0xc0001566) (phy: 0x000000001566) unk. ctxt [_function2]
00016253399: call 0008:c0003dec (0xc0003dec) (phy: 0x000000003dec) unk. ctxt [_function3]
```

## Creating a Slave Disk for File Storage

To support file storage, we also need to create a slave disk `hd80m.img` for *Bochs*.

```console
bochs/bin$ ./bximage -func=create -hd=80 -imgmode=flat -q hd80m.img
```

When it is done, `bximage` shows:

```console
The following line should appear in your bochsrc:
  ata0-master: type=disk, path="hd80m.img", mode=flat
```

We have to change `ata0-master` to `ata0-slave` since we will use it as a slave disk in `bochs/bin/bochsrc.disk`.

```
# Disk
ata0: enabled=1, ioaddr1=0x1f0, ioaddr2=0x3f0, irq=14
# ------------------- bximage output -------------------
ata0-master: type=disk, path="kernel.img", mode=flat
ata0-slave: type=disk, path="hd80m.img", mode=flat
# ------------------------------------------------------
```

### Creating Partitions

`fdisk` can be used to create partitions.

```console
bochs/bin$ fdisk ./hd80m.img
```

Enter <kbd>m</kbd> to show help and enter <kbd>x</kbd> to get expert commands.

```console
Command (m for help): x
Expert command (m for help):
```

Enter <kbd>c</kbd> to set the number of cylinders to `162` and enter <kbd>h</kbd> to set the number of heads to `16`.

```console
Expert command (m for help): c
Number of cylinders (1-1048576, default 10): 162
Expert command (m for help): h
Number of heads (1-255, default 255): 16
```

Enter <kbd>r</kbd> to return to the main menu and enter <kbd>n</kbd> to create partitions.

```console
Command (m for help): n
Partition type
   p   primary (0 primary, 0 extended, 4 free)
   e   extended (container for logical partitions)
Select (default p):
```

Enter <kbd>p</kbd> to create a primary partition.

```console
Select (default p): p
Partition number (1-4, default 1): 1
First sector (2048-163295, default 2048): 2048
Last sector, +/-sectors or +/-size{K,M,G,T,P} (2048-163295, default 163295): 18175

Created a new partition 1 of type 'Linux' and of size 7.9 MiB.
```

Enter <kbd>n</kbd> and <kbd>e</kbd> to create an extended partition.

```console
Select (default p): e
Partition number (2-4, default 2): 4
First sector (18176-163295, default 18432): 18432
Last sector, +/-sectors or +/-size{K,M,G,T,P} (18432-163295, default 163295): 163295

Created a new partition 4 of type 'Extended' and of size 70.7 MiB.
```

Enter <kbd>n</kbd> to add logical partitions.

```console
Command (m for help): n
All space for primary partitions is in use.
Adding logical partition 5
First sector (20480-163295, default 20480): 20480
Last sector, +/-sectors or +/-size{K,M,G,T,P} (20480-163295, default 163295): 29551

Created a new partition 5 of type 'Linux' and of size 4.4 MiB.
```

The first and last sectors of each partition are as follows (<kbd>p</kbd>):

```console
Device     Boot Start    End Sectors  Size Id Type
hd80m.img1       2048  18175   16128  7.9M 83 Linux
hd80m.img4      18432 163295  144864 70.7M  5 Extended
hd80m.img5      20480  29551    9072  4.4M 66 Linux
hd80m.img6      32768  45367   12600  6.2M 66 Linux
```

After creating logical partitions, enter <kbd>t</kbd> to change the type of logical partitions to `66`.

```console
Command (m for help): t
Partition number (1,4-6, default 6): 5
Hex code or alias (type L to list all): 66

Changed type of partition 'Linux' to 'unknown'.
```

Enter <kbd>p</kbd> to print partitions again.

```console
Device     Boot Start    End Sectors  Size Id Type
hd80m.img1       2048  18175   16128  7.9M 83 Linux
hd80m.img4      18432 163295  144864 70.7M  5 Extended
hd80m.img5      20480  29551    9072  4.4M 66 unknown
hd80m.img6      32768  45367   12600  6.2M 66 unknown
```

Finally, enter <kbd>w</kbd> to write configuration to the disk.

```console
Command (m for help): w
The partition table has been altered.
Syncing disks.
```