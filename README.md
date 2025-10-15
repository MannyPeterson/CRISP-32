# CRISP-32 Virtual Machine

Clean RISC Instruction Set Platform - 32-bit Architecture
Freestanding C89 Implementation

## Project Structure

```
CRISP-32/
├── Makefile              # Build system with strict C89 flags
├── crisp32-spec.md       # ISA specification document
├── .vscode/              # VSCode configuration
│   ├── c_cpp_properties.json  # IntelliSense config
│   ├── tasks.json        # Build tasks
│   ├── launch.json       # Debug configurations
│   └── README.md         # VSCode usage guide
├── include/              # Public header files
│   ├── c32_types.h       # Type definitions (no libc)
│   ├── c32_vm.h          # VM structure and API
│   ├── c32_opcodes.h     # Instruction opcode definitions
│   ├── c32_asm.h         # Assembler API
│   └── c32_string.h      # Freestanding string/memory functions
├── src/                  # Source files
│   ├── vm/               # VM sources
│   │   ├── main.c        # VM entry point and test harness
│   │   └── c32_vm.c      # VM core implementation (freestanding)
│   ├── asm/              # Assembler sources
│   │   ├── c32asm.c      # Assembler main program
│   │   ├── c32_parser.c  # Assembly parser
│   │   ├── c32_symbols.c # Symbol table management
│   │   └── c32_encode.c  # Instruction encoding
│   └── common/           # Shared code
│       └── c32_string.c  # String/memory functions
├── tools/                # Development utilities
│   └── bin2h.c           # Binary-to-header converter
├── tests/                # Test assembly programs
│   ├── *.asm             # Assembly source files
│   ├── *.bin             # Assembled binaries (generated)
│   └── *.h               # Embedded test headers (generated)
├── build/                # Object files (generated)
└── bin/                  # Compiled binaries (generated)
    ├── crisp32           # VM executable
    ├── c32asm            # Assembler executable
    └── bin2h             # Binary-to-header converter
```

## Build Instructions

### Command Line
```bash
make                  # Build all (VM + assembler)
make vm               # Build VM only
make asm              # Build assembler only
make debug            # Build with debug symbols
make release          # Explicit release build
make clean            # Clean build artifacts
```

### VSCode
The project includes VSCode configuration files:
- Press `Ctrl+Shift+B` to build
- Press `F5` to debug
- See `.vscode/README.md` for details

### Big-Endian Host
For big-endian hosts, uncomment the flag in Makefile:
```makefile
CFLAGS += -DC32_HOST_BIG_ENDIAN
```

## Compiler Flags

The project uses strict C89 compliance with different flags for different components:

**VM Core** (freestanding):
- `-std=c89` - C89 standard
- `-pedantic` - Strict ISO C compliance
- `-Wall -Wextra -Werror` - All warnings as errors
- `-ffreestanding` - Freestanding environment
- `-fno-builtin` - No compiler built-ins

**Test Harness & Assembler** (hosted):
- `-std=c89 -pedantic -Wall -Wextra -Werror`
- Uses standard library for file I/O and testing

## Features

- **Pure C89**: No C99/C11 features used
- **Freestanding**: No libc dependencies
- **Portable**: Handles endianness automatically
- **Complete**: Implements all required VM operations

## Tools

### C32 Assembler (c32asm)
A two-pass assembler that converts CRISP-32 assembly language to binary machine code.

**Features:**
- Full instruction set support
- Label resolution
- Symbol table management
- Relative branch offset calculation
- Comment support (# and ;)

**Usage:**
```bash
bin/c32asm input.asm output.bin
```

**Example:**
```bash
bin/c32asm tests/hello.asm tests/hello.bin
hexdump -C tests/hello.bin
```

### Binary-to-Header Converter (bin2h)
Converts binary files into C header files for embedded testing in the freestanding VM.

**Purpose:**
Since the VM core is freestanding and cannot use `fopen()`, test programs are embedded as constant arrays. The `bin2h` tool automates this conversion.

**Usage:**
```bash
bin/bin2h input.bin output.h
```

**Build Workflow:**
```
test.asm → (c32asm) → test.bin → (bin2h) → test.h → included in main.c
```

The Makefile automatically generates test headers from binaries:
```bash
make test_headers    # Generate .h files from .bin files
```

See `tests/README.md` for example assembly programs.

## Implementation Status

### VM Core (✅ COMPLETE)
- [x] VM structure definition
- [x] Memory access functions with endianness handling
- [x] Freestanding string/memory functions
- [x] Interrupt management infrastructure
- [x] Instruction fetch, decode, and execute
- [x] All 80+ opcodes implemented
- [x] MMU/paging with virtual address translation
- [x] Privilege level enforcement (kernel/user modes)
- [x] Full interrupt dispatch system
- [x] Page fault handling
- [x] Context save/restore (IRET)

### Assembler (✅ COMPLETE)
- [x] Two-pass assembler
- [x] Full instruction set encoding
- [x] Label and symbol resolution
- [x] Branch offset calculation
- [x] Test programs

### Known Limitations
- **MULH/MULHU**: Return 0 (C89 lacks 64-bit integer support)
- **Floating-point**: Not implemented (reserved opcodes 0x80-0x8F)

## Notes

- The VM implements a 32-bit RISC architecture with 64-bit instructions
- All multi-byte values are stored in little-endian format
- The implementation is completely self-contained with no OS dependencies
