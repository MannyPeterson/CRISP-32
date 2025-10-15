# CRISP-32 Virtual Machine - Strict C89 Build System
# Freestanding implementation (no libc dependencies)

CC = gcc
CFLAGS = -std=c89 -pedantic -Wall -Wextra -Werror
CFLAGS += -ffreestanding -nostdlib -fno-builtin

# Build configuration: set DEBUG=1 for debug build
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O2
endif

# Optional: uncomment for big-endian hosts
# CFLAGS += -DC32_HOST_BIG_ENDIAN

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Source files (to be populated)
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

# Target binary
TARGET = $(BIN_DIR)/crisp32

# Include path
CFLAGS += -I$(INCLUDE_DIR)

.PHONY: all clean directories debug release

all: directories $(TARGET)

debug:
	@$(MAKE) DEBUG=1 all

release:
	@$(MAKE) all

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

# Dependency tracking
-include $(OBJS:.o=.d)

$(BUILD_DIR)/%.d: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	@$(CC) $(CFLAGS) -MM -MT $(BUILD_DIR)/$*.o $< -MF $@
