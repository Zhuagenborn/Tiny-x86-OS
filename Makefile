SHELL := /bin/sh
AS := nasm
LD := ld

DISK := ../kernel.img

INC_DIR := ./include
SRC_DIR := ./src
BUILD_DIR := ./build

vpath %.h $(INC_DIR):$(SRC_DIR)
vpath %.cpp $(SRC_DIR)

vpath %.inc $(INC_DIR):$(SRC_DIR)
vpath %.asm $(SRC_DIR)

vpath %.d $(BUILD_DIR)
vpath %.o $(BUILD_DIR)

CODE_ENTRY := 0xC0001500

LOADER_SECTOR_COUNT := 5
KRNL_START_SECTOR := 6
KRNL_SECTOR_COUNT := 350

ASFLAGS := -f elf \
	-i$(INC_DIR)

# `-fno-pic` can generate position-dependent code that does not use a global pointer register.
# Because the loader does not support address relocation.
# `-O1` can reduce the stack size for local variables. Otherwise threads may have stack overflow errors.
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
LDFLAGS := -m elf_i386 \
	-Ttext $(CODE_ENTRY) \
	-e main

.PHONY: build
build: boot

.PHONY: install
install:
	dd if=$(BUILD_DIR)/boot/mbr.bin of=$(DISK) bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/boot/loader.bin of=$(DISK) seek=1 bs=512 count=$(LOADER_SECTOR_COUNT) conv=notrunc

.PHONY: clean
clean:
	$(RM) $(shell find $(BUILD_DIR) -name '*.d')
	$(RM) $(shell find $(BUILD_DIR) -name '*.o')
	$(RM) $(shell find $(BUILD_DIR) -name '*.bin')
	find $(BUILD_DIR) -empty -type d -delete

########################## Build the master boot record and loader ##########################

BOOT_HEADERS := boot/boot.inc \
	kernel/krnl.inc \
	kernel/util/metric.inc \
	kernel/process/elf.inc \
	kernel/descriptor/desc.inc \
	kernel/selector/sel.inc \
	kernel/io/video/print.inc \
	kernel/io/disk/disk.inc \
	kernel/memory/page.inc

BOOT_SRC := $(SRC_DIR)/boot/mbr.asm $(SRC_DIR)/boot/loader.asm
BOOT_OBJS := $(addprefix $(BUILD_DIR),$(patsubst $(SRC_DIR)%.asm,%.bin,$(BOOT_SRC)))

$(BOOT_OBJS): $(BUILD_DIR)/%.bin: %.asm $(BOOT_HEADERS)
	@mkdir -p $(dir $@)
	$(AS) -i$(INC_DIR) -o $@ $<

.PHONY: boot
boot: $(BOOT_OBJS)