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
│   ├── main.c            # VM entry point
│   ├── c32_vm.c          # VM implementation
│   ├── c32_string.c      # String/memory functions
│   ├── c32asm.c          # Assembler main program
│   ├── c32_parser.c      # Assembly parser
│   ├── c32_symbols.c     # Symbol table management
│   └── c32_encode.c      # Instruction encoding
├── tests/                # Test assembly programs
├── build/                # Object files (generated)
└── bin/                  # Compiled binaries (generated)
    ├── crisp32           # VM executable
    └── c32asm            # Assembler executable
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

The project uses strict C89 compliance:
- `-std=c89` - C89 standard
- `-pedantic` - Strict ISO C compliance
- `-Wall -Wextra -Werror` - All warnings as errors
- `-ffreestanding` - Freestanding environment
- `-nostdlib` - No standard library
- `-fno-builtin` - No compiler built-ins

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

See `tests/README.md` for example assembly programs.

## Implementation Status

### VM Core
- [x] VM structure definition
- [x] Memory access functions with endianness handling
- [x] Freestanding string/memory functions
- [x] Interrupt management infrastructure
- [ ] Instruction decoder
- [ ] Instruction execution
- [ ] MMU/paging support
- [ ] Privilege level enforcement

### Assembler
- [x] Two-pass assembler
- [x] Full instruction set encoding
- [x] Label and symbol resolution
- [x] Branch offset calculation
- [x] Test programs

## Notes

- The VM implements a 32-bit RISC architecture with 64-bit instructions
- All multi-byte values are stored in little-endian format
- The implementation is completely self-contained with no OS dependencies
