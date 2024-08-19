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
build: boot kernel

.PHONY: install
install:
	dd if=$(BUILD_DIR)/boot/mbr.bin of=$(DISK) bs=512 count=1 conv=notrunc
	dd if=$(BUILD_DIR)/boot/loader.bin of=$(DISK) seek=1 bs=512 count=$(LOADER_SECTOR_COUNT) conv=notrunc
	dd if=$(BUILD_DIR)/kernel.bin of=$(DISK) bs=512 seek=$(KRNL_START_SECTOR) count=$(KRNL_SECTOR_COUNT) conv=notrunc

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

############################### Generate C++ dependency files ###############################

CXX_SRC := $(shell find $(SRC_DIR) -name '*.cpp')
CXX_INC_PREQS := $(addprefix $(BUILD_DIR),$(patsubst $(SRC_DIR)%.cpp,%.d,$(CXX_SRC)))

$(CXX_INC_PREQS): $(BUILD_DIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) -MM -I$(INC_DIR) -I$(dir $<) -I$(subst $(SRC_DIR),$(INC_DIR),$(dir $<)) $< > $@
	sed -Ei 's#^(.*\.o: *)$(SRC_DIR:./%=%)/(.*/)?(.*\.cpp)#$(BUILD_DIR:./%=%)/\2\1$(SRC_DIR:./%=%)/\2\3#' $@

include $(CXX_INC_PREQS)

###################################### Build the user #######################################

USR_CXX_SRC := $(shell find $(SRC_DIR)/user -name '*.cpp')
USR_CXX_OBJS := $(addprefix $(BUILD_DIR),$(patsubst $(SRC_DIR)%.cpp,%.o,$(USR_CXX_SRC)))

$(USR_CXX_OBJS): $(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(dir $<) -I$(subst $(SRC_DIR),$(INC_DIR),$(dir $<)) -o $@ $<

.PHONY: user
user: $(USR_CXX_OBJS)

##################################### Build the kernel ######################################

KRNL_AS_HEADERS := $(shell find $(INC_DIR)/kernel -name '*.inc') $(shell find $(SRC_DIR) -name '*.inc')
KRNL_AS_SRC := $(filter-out $(BOOT_SRC),$(shell find $(SRC_DIR) -name '*.asm'))
KRNL_AS_OBJS := $(addprefix $(BUILD_DIR),$(patsubst $(SRC_DIR)%.asm,%_nasm.o,$(KRNL_AS_SRC)))

$(KRNL_AS_OBJS): $(BUILD_DIR)/%_nasm.o: %.asm $(KRNL_AS_HEADERS)
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) -i$(dir $<) -i$(subst $(SRC_DIR),$(INC_DIR),$(dir $<)) -o $@ $<

KRNL_CXX_SRC := $(shell find $(SRC_DIR)/kernel -name '*.cpp')
KRNL_CXX_OBJS := $(addprefix $(BUILD_DIR),$(patsubst $(SRC_DIR)%.cpp,%.o,$(KRNL_CXX_SRC)))

$(KRNL_CXX_OBJS): $(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(dir $<) -I$(subst $(SRC_DIR),$(INC_DIR),$(dir $<)) -o $@ $<

$(BUILD_DIR)/kernel.bin: $(KRNL_AS_OBJS) $(KRNL_CXX_OBJS) $(CRT_CXX_OBJS) $(UTIL_CXX_OBJS) $(USR_CXX_OBJS)
# `main.o` must be the first object file, otherwise another function wil be placed at `0xC0001500`.
	$(LD) $(LDFLAGS) $(BUILD_DIR)/kernel/main.o $(filter-out %/main.o,$^) -o $@

.PHONY: kernel
kernel: $(BUILD_DIR)/kernel.bin