# CRISP-32 Test Programs

This directory contains test assembly programs for the CRISP-32 assembler.

## Test Files

### simple.asm
A minimal test program demonstrating basic instructions:
- ADDI (add immediate)
- ADD (register addition)
- SYSCALL (system call)

**Output**: 32 bytes (4 instructions)

### hello.asm
A more comprehensive test program demonstrating:
- Arithmetic operations (ADD, SUB, MUL)
- Logical operations (AND, OR, XOR)
- Shift operations (SLL, SRL)
- Comparison (SLT)
- Load upper immediate (LUI) and OR immediate
- Branches (BEQ, BGTZ)
- Jumps (JAL, JR)
- Labels and symbol resolution
- Subroutine calls

**Output**: 168 bytes (21 instructions)

## Usage

Assemble a test program:
```bash
bin/c32asm tests/simple.asm tests/simple.bin
```

Verify binary output:
```bash
hexdump -C tests/simple.bin
```

Expected output for simple.asm:
```
00000000  05 00 01 00 2a 00 00 00  05 00 02 00 0a 00 00 00  |....*...........|
00000010  01 01 02 03 00 00 00 00  f0 00 00 00 00 00 00 00  |................|
```

## Instruction Encoding Format

Each instruction is 8 bytes in little-endian format:
```
[opcode][rs][rt][rd][immediate (4 bytes)]
```

Example: `05 00 01 00 2a 00 00 00`
- Opcode: 0x05 (ADDI)
- rs: 0x00 (R0)
- rt: 0x01 (R1)
- rd: 0x00 (unused)
- immediate: 0x0000002A (42)

This encodes: `ADDI R1, R0, 42`
