# CRISP-32 Virtual Machine - Strict C89 Build System
# Builds both the VM (freestanding) and assembler (hosted)

CC = gcc
BASE_CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror

# VM core is freestanding (no libc) but test harness is hosted
VM_CORE_CFLAGS = $(BASE_CFLAGS) -ffreestanding -fno-builtin
VM_TEST_CFLAGS = $(BASE_CFLAGS)

# Assembler uses standard library for file I/O
ASM_CFLAGS = $(BASE_CFLAGS)

# Build configuration: set DEBUG=1 for debug build
ifdef DEBUG
    VM_CORE_CFLAGS += -g -O0 -DDEBUG
    VM_TEST_CFLAGS += -g -O0 -DDEBUG
    ASM_CFLAGS += -g -O0 -DDEBUG
else
    VM_CORE_CFLAGS += -O2
    VM_TEST_CFLAGS += -O2
    ASM_CFLAGS += -O2
endif

# Optional: uncomment for big-endian hosts
# VM_CORE_CFLAGS += -DC32_HOST_BIG_ENDIAN
# VM_TEST_CFLAGS += -DC32_HOST_BIG_ENDIAN
# ASM_CFLAGS += -DC32_HOST_BIG_ENDIAN

# Directories
SRC_DIR = src
VM_SRC = $(SRC_DIR)/vm
ASM_SRC = $(SRC_DIR)/asm
COMMON_SRC = $(SRC_DIR)/common
TEST_SRC = $(SRC_DIR)/test
TOOLS_DIR = tools
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Include path
VM_CORE_CFLAGS += -I$(INCLUDE_DIR)
VM_TEST_CFLAGS += -I$(INCLUDE_DIR)
ASM_CFLAGS += -I$(INCLUDE_DIR)

# VM source files
VM_SRCS = $(VM_SRC)/main.c $(VM_SRC)/c32_vm.c $(COMMON_SRC)/c32_string.c
VM_OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/c32_vm.o $(BUILD_DIR)/c32_string.o

# Assembler source files
ASM_SRCS = $(ASM_SRC)/c32asm.c $(ASM_SRC)/c32_parser.c $(ASM_SRC)/c32_symbols.c \
           $(ASM_SRC)/c32_encode.c $(COMMON_SRC)/c32_string.c $(VM_SRC)/c32_vm.c
ASM_OBJS = $(BUILD_DIR)/c32asm.o $(BUILD_DIR)/c32_parser.o $(BUILD_DIR)/c32_symbols.o \
           $(BUILD_DIR)/c32_encode.o $(BUILD_DIR)/c32_string_asm.o $(BUILD_DIR)/c32_vm_asm.o

# Test suite source files
TEST_SUITE_SRCS = $(TEST_SRC)/test_suite.c $(TEST_SRC)/test_runner.c \
                  $(VM_SRC)/c32_vm.c $(COMMON_SRC)/c32_string.c
TEST_SUITE_OBJS = $(BUILD_DIR)/test_suite.o $(BUILD_DIR)/test_runner.o \
                  $(BUILD_DIR)/c32_vm_test.o $(BUILD_DIR)/c32_string_test.o

# bin2h converter
BIN2H_TARGET = $(BIN_DIR)/bin2h

# Target binaries
VM_TARGET = $(BIN_DIR)/crisp32
ASM_TARGET = $(BIN_DIR)/c32asm
TEST_SUITE_TARGET = $(BIN_DIR)/test_suite

# Test binaries and headers
TEST_DIR = tests
TEST_BINS = $(TEST_DIR)/simple.bin $(TEST_DIR)/hello.bin
TEST_HEADERS = $(TEST_DIR)/simple.h $(TEST_DIR)/hello.h

# Unit test binaries and headers
UNIT_TEST_DIR = $(TEST_DIR)/unit
UNIT_TEST_ASMS = $(wildcard $(UNIT_TEST_DIR)/*.asm)
UNIT_TEST_BINS = $(UNIT_TEST_ASMS:.asm=.bin)
UNIT_TEST_HEADERS = $(UNIT_TEST_ASMS:.asm=.h)

.PHONY: all clean directories debug release vm asm tools test_headers test unit_test_headers test_build

all: directories tools $(VM_TARGET) $(ASM_TARGET)

tools: directories $(BIN2H_TARGET)

vm: directories $(VM_TARGET)

asm: directories $(ASM_TARGET)

debug:
	@$(MAKE) DEBUG=1 all

release:
	@$(MAKE) all

# Build and run unit tests
test: test_build
	@echo "Running unit tests..."
	@$(TEST_SUITE_TARGET)

# Build test suite
test_build: directories tools $(ASM_TARGET) unit_test_headers $(TEST_SUITE_TARGET)

# Generate unit test headers
unit_test_headers: $(UNIT_TEST_HEADERS)

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# VM build rules (core is freestanding, test harness is hosted)
$(VM_TARGET): $(VM_OBJS)
	$(CC) $(VM_TEST_CFLAGS) -o $@ $^

# main.o uses hosted (can use stdio for testing)
$(BUILD_DIR)/main.o: $(VM_SRC)/main.c $(TEST_HEADERS)
	$(CC) $(VM_TEST_CFLAGS) -c -o $@ $<

# Core VM is freestanding
$(BUILD_DIR)/c32_vm.o: $(VM_SRC)/c32_vm.c
	$(CC) $(VM_CORE_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_string.o: $(COMMON_SRC)/c32_string.c
	$(CC) $(VM_CORE_CFLAGS) -c -o $@ $<

# Assembler build rules (hosted, uses libc)
$(ASM_TARGET): $(ASM_OBJS)
	$(CC) $(ASM_CFLAGS) -o $@ $^

$(BUILD_DIR)/c32asm.o: $(ASM_SRC)/c32asm.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_parser.o: $(ASM_SRC)/c32_parser.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_symbols.o: $(ASM_SRC)/c32_symbols.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_encode.o: $(ASM_SRC)/c32_encode.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

# Assembler needs its own builds of shared sources (without freestanding flags)
$(BUILD_DIR)/c32_string_asm.o: $(COMMON_SRC)/c32_string.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_vm_asm.o: $(VM_SRC)/c32_vm.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

# bin2h build rules (hosted)
$(BIN2H_TARGET): $(TOOLS_DIR)/bin2h.c
	$(CC) $(ASM_CFLAGS) -o $@ $<

# Test header generation
test_headers: $(TEST_HEADERS)

$(TEST_DIR)/%.h: $(TEST_DIR)/%.bin $(BIN2H_TARGET)
	$(BIN2H_TARGET) $< $@

# Assemble test programs
$(TEST_DIR)/%.bin: $(TEST_DIR)/%.asm $(ASM_TARGET)
	$(ASM_TARGET) $< $@

# Unit test binary and header generation
$(UNIT_TEST_DIR)/%.bin: $(UNIT_TEST_DIR)/%.asm $(ASM_TARGET)
	$(ASM_TARGET) $< $@

$(UNIT_TEST_DIR)/%.h: $(UNIT_TEST_DIR)/%.bin $(BIN2H_TARGET)
	$(BIN2H_TARGET) $< $@

# Test suite build rules
$(TEST_SUITE_TARGET): $(TEST_SUITE_OBJS)
	$(CC) $(ASM_CFLAGS) -o $@ $^

$(BUILD_DIR)/test_suite.o: $(TEST_SRC)/test_suite.c $(UNIT_TEST_HEADERS)
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/test_runner.o: $(TEST_SRC)/test_runner.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

# Test suite needs its own builds of VM and string (without freestanding flags)
$(BUILD_DIR)/c32_vm_test.o: $(VM_SRC)/c32_vm.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

$(BUILD_DIR)/c32_string_test.o: $(COMMON_SRC)/c32_string.c
	$(CC) $(ASM_CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f $(TEST_DIR)/*.bin $(TEST_DIR)/*.h
	rm -f $(UNIT_TEST_DIR)/*.bin $(UNIT_TEST_DIR)/*.h
