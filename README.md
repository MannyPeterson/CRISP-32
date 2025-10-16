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
│   ├── c32_string.h      # Freestanding string/memory functions
│   └── c32_test.h        # Unit testing framework API
├── src/                  # Source files
│   ├── vm/               # VM sources
│   │   ├── main.c        # VM binary loader (command-line runner)
│   │   └── c32_vm.c      # VM core implementation (freestanding)
│   ├── asm/              # Assembler sources
│   │   ├── c32asm.c      # Assembler main program
│   │   ├── c32_parser.c  # Assembly parser
│   │   ├── c32_symbols.c # Symbol table management
│   │   └── c32_encode.c  # Instruction encoding
│   ├── test/             # Unit test framework
│   │   ├── README.md     # Testing framework documentation
│   │   ├── test_suite.c  # Test definitions and validations
│   │   ├── test_runner.c # Test execution engine
│   │   └── unit/         # Unit test programs
│   │       ├── test_*.asm # Assembly test programs
│   │       ├── test_*.bin # Assembled binaries (generated)
│   │       └── test_*.h   # Embedded test headers (generated)
│   ├── tools/            # Development utilities
│   │   └── bin2h.c       # Binary-to-header converter
│   └── common/           # Shared code
│       └── c32_string.c  # String/memory functions
├── build/                # Object files (generated)
└── bin/                  # Compiled binaries (generated)
    ├── crisp32           # VM executable
    ├── c32asm            # Assembler executable
    ├── test_suite        # Unit test runner
    └── bin2h             # Binary-to-header converter
```

## Build Instructions

### Command Line
```bash
make                  # Build all (VM + assembler)
make vm               # Build VM only
make asm              # Build assembler only
make test             # Build and run unit tests
make test_build       # Build unit tests only (don't run)
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

**VM Runner, Assembler & Test Suite** (hosted):
- `-std=c89 -pedantic -Wall -Wextra -Werror`
- Uses standard library for file I/O, testing, and user interaction

## Features

- **Pure C89**: No C99/C11 features used
- **Freestanding**: No libc dependencies
- **Portable**: Handles endianness automatically
- **Complete**: Implements all required VM operations

## Tools

### CRISP-32 Virtual Machine (crisp32)
Command-line VM that loads and executes CRISP-32 binary programs.

**Usage:**
```bash
bin/crisp32 <binary_file> [load_address]
```

**Example:**
```bash
# Assemble a program
bin/c32asm program.asm program.bin

# Run it in the VM
bin/crisp32 program.bin

# Load at custom address
bin/crisp32 program.bin 0x2000
```

**Output:**
The VM displays execution statistics and final register state:
```
Loaded 32 bytes from 'program.bin' at address 0x00001000

Starting execution at 0x00001000...

Program halted after 4 steps

Register State:
================
R0 : 0x00000000  R1 : 0x00000064  R2 : 0x00000032  R3 : 0x00000096
...
```

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
# Create a simple program
cat > add.asm << 'EOF'
ADDI R1, R0, 100
ADDI R2, R0, 50
ADD R3, R1, R2
SYSCALL
EOF

# Assemble it
bin/c32asm add.asm add.bin

# Run it
bin/crisp32 add.bin
```

### Binary-to-Header Converter (bin2h)
Converts binary files into C header files for embedding in the unit test framework.

**Purpose:**
Test programs are embedded as constant arrays in the test suite. The `bin2h` tool automates this conversion.

**Usage:**
```bash
bin/bin2h input.bin output.h
```

**Build Workflow:**
```
test.asm → (c32asm) → test.bin → (bin2h) → test.h → included in test_suite.c
```

The Makefile automatically generates test headers from binaries:
```bash
make unit_test_headers    # Generate .h files from .bin files
```

## Unit Testing Framework

CRISP-32 includes a comprehensive JUnit-style unit testing framework for validating VM instruction execution.

**Features:**
- Assembly test programs with expected outcomes
- Assertion macros for register, memory, and VM state validation
- Automated test assembly and embedding
- Pass/fail reporting with detailed error messages

**Quick Start:**
```bash
# Run all unit tests
make test

# View test output
Running test suite (7 tests)...

[1/7] ADD and ADDI instructions ... PASS
[2/7] SUB instruction ... PASS
[3/7] MUL instruction ... PASS
...
```

**Test Coverage:**
- ✅ Arithmetic: ADD, ADDI, SUB, MUL
- ✅ Logical: AND, OR, XOR
- ✅ Shifts: SLL, SRL
- Branch and memory operations (tests written, awaiting implementation)

**Adding New Tests:**

1. Create assembly test program in `src/test/unit/`
2. Add validation function in `src/test/test_suite.c`
3. Register test case in test suite array
4. Run `make test`

See `src/test/README.md` for complete documentation.

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
- [x] Binary file output

### Unit Testing Framework (✅ COMPLETE)
- [x] JUnit-style test framework
- [x] Test runner with pass/fail reporting
- [x] Assertion macros (register, memory, PC, VM state)
- [x] Automated test assembly and embedding
- [x] Example unit tests for all instruction categories
- [x] Comprehensive test documentation

### Known Limitations
- **MULH/MULHU**: Return 0 (C89 lacks 64-bit integer support)
- **Floating-point**: Not implemented (reserved opcodes 0x80-0x8F)

## Notes

- The VM implements a 32-bit RISC architecture with 64-bit instructions
- All multi-byte values are stored in little-endian format
- The implementation is completely self-contained with no OS dependencies
