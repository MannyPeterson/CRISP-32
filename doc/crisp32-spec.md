# CRISP-32 ISA Specification
**Clean RISC Instruction Set Platform - 32-bit Architecture**

**Version:** 2.0
**Author:** Manny Peterson
**Date:** 2025
**Implementation:** Freestanding C89 Virtual Machine

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Architecture Overview](#2-architecture-overview)
3. [Instruction Format](#3-instruction-format)
4. [Instruction Set Reference](#4-instruction-set-reference)
5. [Memory Architecture](#5-memory-architecture)
6. [Privilege Levels and Protection](#6-privilege-levels-and-protection)
7. [Virtual Memory and Paging](#7-virtual-memory-and-paging)
8. [Interrupt System](#8-interrupt-system)
9. [VM Implementation](#9-vm-implementation)
10. [Assembly Language](#10-assembly-language)
11. [Programming Guide](#11-programming-guide)
12. [Design Rationale](#12-design-rationale)
13. [Appendices](#13-appendices)

---

## 1. Introduction

### 1.1 Overview

CRISP-32 (C32) is a clean, simple RISC (Reduced Instruction Set Computer) instruction set architecture designed for maximum portability across embedded systems and as a teaching platform for computer architecture. It is implemented as a virtual machine written in strict freestanding C89, serving as a hypervisor that provides 100% code portability without requiring a host operating system.

Unlike traditional ISAs designed for hardware implementation, CRISP-32 is optimized for software virtual machines while maintaining hardware implementability. The architecture prioritizes:

- **Simplicity**: Uniform instruction encoding, minimal special cases
- **Portability**: Freestanding C89 implementation with no dependencies
- **Completeness**: Full privilege separation, virtual memory, and interrupt handling
- **Performance**: Efficient VM implementation with straightforward decode logic
- **Extensibility**: Large opcode space for future extensions

**Implementation Source:** `src/vm/c32_vm.c`, `include/c32_vm.h`

### 1.2 Design Philosophy

The CRISP-32 design follows several key principles:

1. **No Hidden State**: All architectural state is explicit and accessible
2. **Uniform Encoding**: All instructions use the same 64-bit format
3. **Load-Store Architecture**: Memory access only through explicit load/store instructions
4. **No Delay Slots**: Branches and jumps take effect immediately
5. **Simple Privilege Model**: Two levels (kernel/user) sufficient for most use cases
6. **Optional Protection**: Can run with or without memory protection
7. **Freestanding Implementation**: Zero dependencies on standard libraries or OS services

### 1.3 Target Applications

CRISP-32 is designed for:

- **Embedded Systems**: Bare-metal firmware development
- **Operating Systems**: Kernel development and system programming
- **Education**: Teaching computer architecture and systems programming
- **Virtualization**: Application sandboxing and isolation
- **Portability**: Write-once, run-anywhere code execution

### 1.4 Document Conventions

This specification uses the following notation:

- **Hexadecimal**: `0x` prefix (e.g., `0x1000`)
- **Binary**: `0b` prefix (e.g., `0b1010`)
- **Bit Fields**: `[msb:lsb]` (e.g., `[31:12]` means bits 31 down to 12)
- **Memory**: `mem[addr]` indicates memory at address `addr`
- **Registers**: `Rn` where n is 0-31, or symbolic names (`sp`, `ra`, etc.)
- **Signed Extension**: `sign_ext(value)` means sign-extending to 32 bits
- **Zero Extension**: `zero_ext(value)` means zero-extending to 32 bits

---

## 2. Architecture Overview

### 2.1 Core Architecture

CRISP-32 is a 32-bit RISC architecture with the following characteristics:

**Data Formats:**
- **Byte**: 8 bits
- **Halfword**: 16 bits (2 bytes)
- **Word**: 32 bits (4 bytes) - native register and address size
- **Instruction**: 64 bits (8 bytes) - fixed-length encoding

**Byte Order:**
- **Little-Endian**: All multi-byte values stored LSB first
- **Automatic Conversion**: VM handles big-endian hosts transparently
- **Implementation**: `src/vm/c32_vm.c` lines 36-175 (endianness conversion)

**Address Space:**
- **32-bit Addressing**: 4 GB (0x00000000 - 0xFFFFFFFF)
- **Byte-Addressable**: Each byte has unique address
- **Aligned Access**: Word (4-byte), halfword (2-byte) alignment required

### 2.2 Register Set

CRISP-32 provides 32 general-purpose registers plus a program counter:

#### 2.2.1 General Purpose Registers

| Register | Number | ABI Name | Usage | Preserved? |
|----------|--------|----------|-------|------------|
| R0 | 0 | zero | Constant zero (hardwired) | N/A |
| R1 | 1 | at | Assembler temporary | No |
| R2-R3 | 2-3 | v0-v1 | Function return values | No |
| R4-R7 | 4-7 | a0-a3 | Function arguments | No |
| R8-R15 | 8-15 | t0-t7 | Temporary registers | No |
| R16-R23 | 16-23 | s0-s7 | Saved registers | Yes |
| R24-R25 | 24-25 | t8-t9 | More temporaries | No |
| R26-R27 | 26-27 | k0-k1 | Kernel reserved | No |
| R28 | 28 | gp | Global pointer (optional) | Yes |
| R29 | 29 | sp | Stack pointer (optional) | Yes |
| R30 | 30 | fp | Frame pointer (optional) | Yes |
| R31 | 31 | ra | Return address | No |

**R0 Special Behavior:**
- Always reads as zero
- Writes are silently ignored
- Enforced by VM after every instruction (`src/vm/c32_vm.c` line 945)
- Useful for pseudo-instructions (e.g., `NOP` can be `OR R0, R0, R0`)

**Register Usage Notes:**
- **ABI Names**: Software conventions only, not enforced by hardware
- **Preservation**: Caller-saved vs callee-saved is convention
- **R31 (ra)**: Automatically set by `JAL`/`JALR` instructions
- **R29 (sp)**: Used by interrupt handling for context save
- **R26-R27 (k0-k1)**: Reserved for kernel use (interrupt handlers)

**Implementation:** `c32_vm_t.regs[32]` in `include/c32_vm.h` line 45

#### 2.2.2 Program Counter (PC)

- **Size**: 32 bits
- **Alignment**: Must be 8-byte aligned (bits [2:0] must be zero)
- **Increment**: Advances by 8 bytes per instruction
- **Branches**: PC-relative offsets are byte offsets, not instruction offsets
- **Jumps**: Target address must be 8-byte aligned

**PC Misalignment Handling:**
- Checked before every instruction fetch (`src/vm/c32_vm.c` line 538)
- Raises interrupt 2 (MEM_FAULT) if misaligned
- VM halts on misalignment if no handler installed

**Implementation:** `c32_vm_t.pc` in `include/c32_vm.h` line 48

### 2.3 Memory Model

#### 2.3.1 Address Space Layout

```
Physical Address Space (Example 16MB system):
┌────────────────────────────────────────────┐
│ 0x00000000 - 0x000007FF                    │  Interrupt Vector Table (IVT)
│ 2048 bytes (256 entries × 8 bytes)         │  - Entry N at offset N × 8
│                                            │  - Handler address (4 bytes)
│                                            │  - Reserved (4 bytes)
├────────────────────────────────────────────┤
│ 0x00000800 - 0x00000FFF                    │  Reserved
│ 2048 bytes                                 │  - Available for future use
├────────────────────────────────────────────┤
│ 0x00001000 - 0x00008FFF                    │  Kernel Code & Data
│ 32 KB                                      │  - Firmware, bootloader
│                                            │  - Interrupt handlers
│                                            │  - System data structures
├────────────────────────────────────────────┤
│ 0x00009000 - 0x00009FFF                    │  Page Table (optional)
│ 4 KB                                       │  - 1024 entries for 4MB virtual space
│                                            │  - Each entry: 4 bytes
├────────────────────────────────────────────┤
│ 0x0000A000 - 0x00FFFFFF                    │  User Memory / Free RAM
│ ~16 MB                                     │  - Application code
│                                            │  - Data, heap, stack
└────────────────────────────────────────────┘
```

**Key Address Ranges:**
- **IVT Base**: Always 0x00000000 (not relocatable)
- **Reset Vector**: Typically 0x00001000 (first instruction)
- **User Code**: Can start anywhere after kernel space
- **Stack**: Typically grows down from high memory

#### 2.3.2 Alignment Requirements

| Access Type | Alignment | Consequence if Misaligned |
|-------------|-----------|---------------------------|
| Byte (LB, LBU, SB) | None (1-byte) | N/A - always aligned |
| Halfword (LH, LHU, SH) | 2-byte | Undefined behavior |
| Word (LW, SW) | 4-byte | Undefined behavior |
| Instruction Fetch | 8-byte | MEM_FAULT interrupt |

**Implementation Notes:**
- Current VM does not enforce data alignment (implementation-defined)
- Misaligned access may work but is not guaranteed
- Portable code should always use aligned access

#### 2.3.3 Endianness

All multi-byte values in CRISP-32 memory are stored in **little-endian** format:

**Example: 32-bit value 0x12345678 at address 0x1000**
```
Address    Value
0x1000:    0x78   (LSB)
0x1001:    0x56
0x1002:    0x34
0x1003:    0x12   (MSB)
```

**Big-Endian Host Support:**
- Compile with `-DC32_HOST_BIG_ENDIAN` flag
- VM automatically byte-swaps on load/store
- Implementation: `src/vm/c32_vm.c` lines 36-175
- Functions: `swap32()`, `swap16()` for conversion

### 2.4 Data Types

| Type | Size | Signed Range | Unsigned Range | C Type |
|------|------|--------------|----------------|--------|
| Byte | 8 bits | -128 to 127 | 0 to 255 | int8_t, uint8_t |
| Halfword | 16 bits | -32,768 to 32,767 | 0 to 65,535 | int16_t, uint16_t |
| Word | 32 bits | -2,147,483,648 to 2,147,483,647 | 0 to 4,294,967,295 | int32_t, uint32_t |

**Type Definitions:** `include/c32_types.h` lines 34-60

**Sign Extension Examples:**
```
LB R1, 0(R0)    ; Load signed byte 0xFF
                ; R1 = 0xFFFFFFFF (-1)

LBU R1, 0(R0)   ; Load unsigned byte 0xFF
                ; R1 = 0x000000FF (255)

LH R1, 0(R0)    ; Load signed halfword 0x8000
                ; R1 = 0xFFFF8000 (-32768)

LHU R1, 0(R0)   ; Load unsigned halfword 0x8000
                ; R1 = 0x00008000 (32768)
```

**Implementation:** `src/vm/c32_vm.c` lines 735-775 (load operations)

---

## 3. Instruction Format

### 3.1 Uniform 64-bit Encoding

All CRISP-32 instructions are exactly **64 bits (8 bytes)** with a uniform encoding scheme. This eliminates the need for variable-length instructions and simplifies decode logic.

**Bit Layout:**
```
 Bits:   63    56 55    48 47    40 39    32 31                    0
        ┌─────────┬─────────┬─────────┬─────────┬────────────────────┐
        │ opcode  │   rs    │   rt    │   rd    │     immediate      │
        │ 8 bits  │ 8 bits  │ 8 bits  │ 8 bits  │     32 bits        │
        └─────────┴─────────┴─────────┴─────────┴────────────────────┘
```

**Memory Layout (Little-Endian):**
```
Offset:  +0      +1      +2      +3      +4      +5      +6      +7
        ┌───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┐
        │opcode │  rs   │  rt   │  rd   │     immediate (LSB→MSB)       │
        └───────┴───────┴───────┴───────┴───────┴───────┴───────┴───────┘
```

**Field Descriptions:**

| Field | Bits | Size | Description | Usage |
|-------|------|------|-------------|-------|
| opcode | [63:56] | 8 bits | Operation code | 256 possible instructions |
| rs | [55:48] | 8 bits | Source register 1 | Register index 0-31 (5 bits used) |
| rt | [47:40] | 8 bits | Source register 2 / target | Register index 0-31 |
| rd | [39:32] | 8 bits | Destination register | Register index 0-31 |
| immediate | [31:0] | 32 bits | Immediate value / offset / address | Full 32-bit immediate |

**Implementation:** `src/vm/c32_vm.c` lines 558-564 (instruction decode)

### 3.2 Instruction Types

While all instructions use the same format, they can be categorized by field usage:

#### 3.2.1 R-Type (Register-Register)

Used for operations involving three registers.

```
Format: OPCODE rd, rs, rt
Example: ADD R5, R10, R12

Fields:
  opcode = operation (e.g., 0x01 for ADD)
  rs = first source operand (R10)
  rt = second source operand (R12)
  rd = destination (R5)
  imm = unused (ignored)

Operation: R[rd] = R[rs] OP R[rt]
```

**Instructions:** ADD, SUB, AND, OR, XOR, NOR, SLT, SLTU, MUL, DIV, etc.

#### 3.2.2 I-Type (Register-Immediate)

Used for operations with one register and one immediate value.

```
Format: OPCODE rt, rs, immediate
Example: ADDI R3, R17, 42

Fields:
  opcode = operation (e.g., 0x05 for ADDI)
  rs = source operand (R17)
  rt = destination (R3)
  rd = unused (ignored)
  imm = 32-bit immediate value (42)

Operation: R[rt] = R[rs] OP imm
```

**Instructions:** ADDI, ANDI, ORI, XORI, SLTI, SLTIU, LUI

#### 3.2.3 M-Type (Memory Access)

Used for load and store operations.

```
Format: OPCODE rt, offset(rs)
Example: LW R8, 100(R2)

Fields:
  opcode = operation (e.g., 0x50 for LW)
  rs = base address register (R2)
  rt = destination/source register (R8)
  rd = unused (ignored)
  imm = signed 32-bit offset (100)

Operation (Load):  R[rt] = mem[R[rs] + imm]
Operation (Store): mem[R[rs] + imm] = R[rt]
```

**Instructions:** LW, LH, LHU, LB, LBU, SW, SH, SB

#### 3.2.4 B-Type (Branch)

Used for conditional branches.

```
Format: OPCODE rs, rt, offset
Example: BEQ R5, R6, 0x100

Fields:
  opcode = branch condition (e.g., 0x60 for BEQ)
  rs = first compare operand (R5)
  rt = second compare operand (R6)
  rd = unused (ignored)
  imm = signed 32-bit byte offset (0x100)

Operation: if (R[rs] COND R[rt]) PC = PC + imm
```

**Branch Offset:**
- Byte offset, not instruction offset
- Relative to PC **after** incrementing (PC+8)
- Example: `BEQ R0, R0, -8` branches to current instruction (infinite loop)

**Instructions:** BEQ, BNE, BLEZ, BGTZ, BLTZ, BGEZ

#### 3.2.5 J-Type (Jump)

Used for unconditional jumps.

```
Format: OPCODE target
Example: JAL 0x1000

Fields:
  opcode = jump type (e.g., 0x71 for JAL)
  rs = unused (or source for JR/JALR)
  rt = unused
  rd = link register (or unused for J/JR)
  imm = absolute 32-bit target address (0x1000)

Operation (J):    PC = imm
Operation (JAL):  R[31] = PC; PC = imm
Operation (JR):   PC = R[rs]
Operation (JALR): R[rd] = PC; PC = R[rs]
```

**Instructions:** J, JAL, JR, JALR

### 3.3 Encoding Examples

#### Example 1: ADD R5, R10, R12

```
Assembly:  ADD R5, R10, R12
Operation: R5 = R10 + R12

Encoding:
  opcode = 0x01 (ADD)
  rs = 0x0A (R10)
  rt = 0x0C (R12)
  rd = 0x05 (R5)
  imm = 0x00000000 (unused)

Bytes (little-endian): 01 0A 0C 05 00 00 00 00

Memory View:
Address  +0  +1  +2  +3  +4  +5  +6  +7
0x1000:  01  0A  0C  05  00  00  00  00
```

#### Example 2: ADDI R3, R17, 42

```
Assembly:  ADDI R3, R17, 42
Operation: R3 = R17 + 42

Encoding:
  opcode = 0x05 (ADDI)
  rs = 0x11 (R17)
  rt = 0x03 (R3)
  rd = 0x00 (unused)
  imm = 0x0000002A (42)

Bytes (little-endian): 05 11 03 00 2A 00 00 00

Memory View:
Address  +0  +1  +2  +3  +4  +5  +6  +7
0x1008:  05  11  03  00  2A  00  00  00
```

#### Example 3: LW R8, 100(R2)

```
Assembly:  LW R8, 100(R2)
Operation: R8 = mem[R2 + 100]

Encoding:
  opcode = 0x50 (LW)
  rs = 0x02 (R2 - base register)
  rt = 0x08 (R8 - destination)
  rd = 0x00 (unused)
  imm = 0x00000064 (100)

Bytes (little-endian): 50 02 08 00 64 00 00 00

Memory View:
Address  +0  +1  +2  +3  +4  +5  +6  +7
0x1010:  50  02  08  00  64  00  00  00
```

#### Example 4: BEQ R5, R6, 0x100

```
Assembly:  BEQ R5, R6, 0x100
Operation: if (R5 == R6) PC = PC + 0x100

Encoding:
  opcode = 0x60 (BEQ)
  rs = 0x05 (R5)
  rt = 0x06 (R6)
  rd = 0x00 (unused)
  imm = 0x00000100 (256 byte offset)

Bytes (little-endian): 60 05 06 00 00 01 00 00

Memory View:
Address  +0  +1  +2  +3  +4  +5  +6  +7
0x1018:  60  05  06  00  00  01  00  00
```

#### Example 5: JAL 0x1000

```
Assembly:  JAL 0x1000
Operation: R31 = PC; PC = 0x1000

Encoding:
  opcode = 0x71 (JAL)
  rs = 0x00 (unused)
  rt = 0x00 (unused)
  rd = 0x00 (unused, R31 implicit)
  imm = 0x00001000 (target address)

Bytes (little-endian): 71 00 00 00 00 10 00 00

Memory View:
Address  +0  +1  +2  +3  +4  +5  +6  +7
0x1020:  71  00  00  00  00  10  00  00
```

### 3.4 Why 64-bit Instructions?

The decision to use 64-bit fixed-length instructions provides several advantages:

**Benefits:**
1. **Full 32-bit Immediates**: Can load any constant in one instruction (no LUI/ORI sequence needed)
2. **Simplified Decode**: All fields at fixed offsets
3. **No Instruction Fusion**: No need for multi-instruction sequences
4. **Large Opcode Space**: 256 opcodes (vs 32-64 in 32-bit RISC)
5. **Future Expansion**: Can extend to 256 registers if needed

**Trade-offs:**
- **Code Density**: 2x larger than 32-bit instructions
- **Memory Bandwidth**: More fetch bandwidth required
- **Cache Impact**: Fewer instructions per cache line

**Justification**: For a VM-based ISA, the decode simplicity and immediate flexibility outweigh code size concerns. The VM can prefetch and cache decoded instructions for performance.

---

## 4. Instruction Set Reference

This section provides complete documentation for all 80+ instructions in the CRISP-32 ISA.

**Opcode Definitions:** `include/c32_opcodes.h`
**Implementation:** `src/vm/c32_vm.c` lines 570-942

### 4.1 Arithmetic Instructions

#### 4.1.1 ADD - Add Signed

```
Format:  ADD rd, rs, rt
Opcode:  0x01
Type:    R-Type
Syntax:  rd = rs + rt (signed)

Description:
  Adds the signed 32-bit values in rs and rt, stores result in rd.
  Overflow is not detected.

Operation:
  R[rd] = (int32_t)R[rs] + (int32_t)R[rt]

Exceptions: None

Example:
  ADD R5, R10, R12  ; R5 = R10 + R12
  ; If R10 = 100, R12 = 50, then R5 = 150

Assembly Encoding:
  01 0A 0C 05 00 00 00 00

Implementation: src/vm/c32_vm.c line 576
```

#### 4.1.2 ADDU - Add Unsigned

```
Format:  ADDU rd, rs, rt
Opcode:  0x02
Type:    R-Type
Syntax:  rd = rs + rt (unsigned)

Description:
  Adds the unsigned 32-bit values in rs and rt, stores result in rd.
  Overflow is not detected.

Operation:
  R[rd] = (uint32_t)R[rs] + (uint32_t)R[rt]

Exceptions: None

Example:
  ADDU R5, R10, R12  ; R5 = R10 + R12 (unsigned)

Implementation: src/vm/c32_vm.c line 579
```

#### 4.1.3 ADDI - Add Immediate Signed

```
Format:  ADDI rt, rs, immediate
Opcode:  0x05
Type:    I-Type
Syntax:  rt = rs + imm (signed)

Description:
  Adds the signed immediate value to the signed value in rs,
  stores result in rt. The immediate is sign-extended to 32 bits.

Operation:
  R[rt] = (int32_t)R[rs] + (int32_t)imm

Exceptions: None

Example:
  ADDI R3, R17, 42    ; R3 = R17 + 42
  ADDI R5, R5, -10    ; R5 = R5 - 10
  ; Negative immediates are represented in two's complement

Usage Notes:
  - Common for stack pointer adjustment
  - Can add constants up to ±2^31

Implementation: src/vm/c32_vm.c line 590
```

#### 4.1.4 ADDIU - Add Immediate Unsigned

```
Format:  ADDIU rt, rs, immediate
Opcode:  0x06
Type:    I-Type
Syntax:  rt = rs + imm (unsigned)

Description:
  Adds the immediate value (treated as unsigned) to rs,
  stores result in rt. No overflow detection.

Operation:
  R[rt] = (uint32_t)R[rs] + (uint32_t)imm

Exceptions: None

Example:
  ADDIU R3, R17, 42  ; R3 = R17 + 42 (unsigned)

Implementation: src/vm/c32_vm.c line 593
```

#### 4.1.5 SUB - Subtract Signed

```
Format:  SUB rd, rs, rt
Opcode:  0x03
Type:    R-Type
Syntax:  rd = rs - rt (signed)

Description:
  Subtracts the signed value in rt from rs, stores result in rd.
  Overflow is not detected.

Operation:
  R[rd] = (int32_t)R[rs] - (int32_t)R[rt]

Exceptions: None

Example:
  SUB R3, R10, R5   ; R3 = R10 - R5
  ; If R10 = 100, R5 = 30, then R3 = 70

Implementation: src/vm/c32_vm.c line 582
```

#### 4.1.6 SUBU - Subtract Unsigned

```
Format:  SUBU rd, rs, rt
Opcode:  0x04
Type:    R-Type
Syntax:  rd = rs - rt (unsigned)

Description:
  Subtracts the unsigned value in rt from rs, stores result in rd.

Operation:
  R[rd] = (uint32_t)R[rs] - (uint32_t)R[rt]

Exceptions: None

Example:
  SUBU R3, R10, R5  ; R3 = R10 - R5 (unsigned)

Implementation: src/vm/c32_vm.c line 585
```

#### 4.1.7 MUL - Multiply

```
Format:  MUL rd, rs, rt
Opcode:  0x40
Type:    R-Type
Syntax:  rd = (rs * rt)[31:0]

Description:
  Multiplies rs and rt, stores the lower 32 bits of the 64-bit
  result in rd. The upper 32 bits are discarded.

Operation:
  R[rd] = (uint32_t)((uint64_t)R[rs] * (uint64_t)R[rt])

Exceptions: None

Example:
  MUL R3, R7, R6    ; R3 = (R7 * R6) & 0xFFFFFFFF
  ; If R7 = 7, R6 = 6, then R3 = 42

Usage Notes:
  - Result is same for signed/unsigned multiplication (lower 32 bits)
  - For full 64-bit result, use MULH/MULHU for upper bits

Implementation: src/vm/c32_vm.c line 664
```

#### 4.1.8 MULH - Multiply High Signed

```
Format:  MULH rd, rs, rt
Opcode:  0x41
Type:    R-Type
Syntax:  rd = (rs * rt)[63:32] (signed)

Description:
  Multiplies rs and rt as signed values, stores the upper 32 bits
  of the 64-bit result in rd.

Operation:
  product = (int64_t)(int32_t)R[rs] * (int64_t)(int32_t)R[rt]
  R[rd] = (uint32_t)(product >> 32)

Exceptions: None

Example:
  MULH R10, R5, R6  ; R10 = upper 32 bits of (R5 * R6)
  ; For 65536 * 65536 = 4294967296 (0x100000000)
  ; MULH returns 1 (0x00000001)

C89 Implementation:
  Due to lack of 64-bit types in C89, implemented using
  32-bit arithmetic with Karatsuba-like algorithm.

Implementation: src/vm/c32_vm.c lines 667-688
```

#### 4.1.9 MULHU - Multiply High Unsigned

```
Format:  MULHU rd, rs, rt
Opcode:  0x42
Type:    R-Type
Syntax:  rd = (rs * rt)[63:32] (unsigned)

Description:
  Multiplies rs and rt as unsigned values, stores the upper 32 bits
  of the 64-bit result in rd.

Operation:
  product = (uint64_t)R[rs] * (uint64_t)R[rt]
  R[rd] = (uint32_t)(product >> 32)

Exceptions: None

Example:
  MULHU R10, R5, R6  ; R10 = upper 32 bits of (R5 * R6) unsigned

Implementation: src/vm/c32_vm.c lines 689-704
```

#### 4.1.10 DIV - Divide Signed

```
Format:  DIV rd, rs, rt
Opcode:  0x43
Type:    R-Type
Syntax:  rd = rs / rt (signed)

Description:
  Divides rs by rt as signed integers, stores quotient in rd.
  Division by zero returns 0 (no exception).

Operation:
  if (R[rt] != 0)
    R[rd] = (int32_t)R[rs] / (int32_t)R[rt]
  else
    R[rd] = 0

Exceptions: None (division by zero returns 0)

Example:
  DIV R3, R10, R7   ; R3 = R10 / R7
  ; If R10 = 100, R7 = 7, then R3 = 14

Special Cases:
  - Division by zero: returns 0
  - -2147483648 / -1: returns -2147483648 (overflow case)

Implementation: src/vm/c32_vm.c lines 705-711
```

#### 4.1.11 DIVU - Divide Unsigned

```
Format:  DIVU rd, rs, rt
Opcode:  0x44
Type:    R-Type
Syntax:  rd = rs / rt (unsigned)

Description:
  Divides rs by rt as unsigned integers, stores quotient in rd.
  Division by zero returns 0.

Operation:
  if (R[rt] != 0)
    R[rd] = (uint32_t)R[rs] / (uint32_t)R[rt]
  else
    R[rd] = 0

Exceptions: None

Example:
  DIVU R3, R10, R7  ; R3 = R10 / R7 (unsigned)

Implementation: src/vm/c32_vm.c lines 712-718
```

#### 4.1.12 REM - Remainder Signed

```
Format:  REM rd, rs, rt
Opcode:  0x45
Type:    R-Type
Syntax:  rd = rs % rt (signed)

Description:
  Computes the signed remainder of rs divided by rt, stores in rd.
  Division by zero returns 0.

Operation:
  if (R[rt] != 0)
    R[rd] = (int32_t)R[rs] % (int32_t)R[rt]
  else
    R[rd] = 0

Exceptions: None

Example:
  REM R4, R10, R7   ; R4 = R10 % R7
  ; If R10 = 100, R7 = 7, then R4 = 2
  ; (100 = 14*7 + 2)

Sign Rules:
  - Result has same sign as dividend (rs)
  - (-100) % 7 = -2
  - 100 % (-7) = 2

Implementation: src/vm/c32_vm.c lines 719-725
```

#### 4.1.13 REMU - Remainder Unsigned

```
Format:  REMU rd, rs, rt
Opcode:  0x46
Type:    R-Type
Syntax:  rd = rs % rt (unsigned)

Description:
  Computes the unsigned remainder of rs divided by rt, stores in rd.
  Division by zero returns 0.

Operation:
  if (R[rt] != 0)
    R[rd] = (uint32_t)R[rs] % (uint32_t)R[rt]
  else
    R[rd] = 0

Exceptions: None

Example:
  REMU R4, R10, R7  ; R4 = R10 % R7 (unsigned)

Implementation: src/vm/c32_vm.c lines 726-732
```

### 4.2 Logical Instructions

#### 4.2.1 AND - Bitwise AND

```
Format:  AND rd, rs, rt
Opcode:  0x10
Type:    R-Type
Syntax:  rd = rs & rt

Description:
  Performs bitwise AND of rs and rt, stores result in rd.

Operation:
  R[rd] = R[rs] & R[rt]

Exceptions: None

Example:
  AND R3, R1, R2
  ; If R1 = 0x0F (0000 1111), R2 = 0x33 (0011 0011)
  ; Then R3 = 0x03 (0000 0011)

Common Uses:
  - Masking bits
  - Testing bit flags
  - Clearing bits with inverted mask

Implementation: src/vm/c32_vm.c line 598
```

#### 4.2.2 OR - Bitwise OR

```
Format:  OR rd, rs, rt
Opcode:  0x11
Type:    R-Type
Syntax:  rd = rs | rt

Description:
  Performs bitwise OR of rs and rt, stores result in rd.

Operation:
  R[rd] = R[rs] | R[rt]

Exceptions: None

Example:
  OR R4, R1, R2
  ; If R1 = 0x0F (0000 1111), R2 = 0x33 (0011 0011)
  ; Then R4 = 0x3F (0011 1111)

Special Case:
  OR R5, R10, R0  ; Copies R10 to R5 (equivalent to MOV)

Implementation: src/vm/c32_vm.c line 601
```

#### 4.2.3 XOR - Bitwise XOR

```
Format:  XOR rd, rs, rt
Opcode:  0x12
Type:    R-Type
Syntax:  rd = rs ^ rt

Description:
  Performs bitwise exclusive-OR of rs and rt, stores result in rd.

Operation:
  R[rd] = R[rs] ^ R[rt]

Exceptions: None

Example:
  XOR R5, R1, R2
  ; If R1 = 0x0F (0000 1111), R2 = 0x33 (0011 0011)
  ; Then R5 = 0x3C (0011 1100)

Common Uses:
  - Toggling bits
  - Zero detection: XOR R5, R10, R10 ; R5 = 0
  - Comparing values (result zero if equal)

Implementation: src/vm/c32_vm.c line 604
```

#### 4.2.4 NOR - Bitwise NOR

```
Format:  NOR rd, rs, rt
Opcode:  0x13
Type:    R-Type
Syntax:  rd = ~(rs | rt)

Description:
  Performs bitwise NOR (NOT OR) of rs and rt, stores result in rd.

Operation:
  R[rd] = ~(R[rs] | R[rt])

Exceptions: None

Example:
  NOR R5, R1, R2
  ; If R1 = 0x0F, R2 = 0x33, then R5 = ~0x3F = 0xFFFFFFC0

Special Case:
  NOR R5, R10, R0  ; Bitwise NOT of R10 (equivalent to NOT)

Implementation: src/vm/c32_vm.c line 607
```

#### 4.2.5 ANDI - AND Immediate

```
Format:  ANDI rt, rs, immediate
Opcode:  0x14
Type:    I-Type
Syntax:  rt = rs & imm

Description:
  Performs bitwise AND of rs and zero-extended immediate,
  stores result in rt.

Operation:
  R[rt] = R[rs] & imm

Exceptions: None

Example:
  ANDI R5, R10, 0xFF  ; Mask lower 8 bits
  ; If R10 = 0x12345678, then R5 = 0x00000078

Common Uses:
  - Extracting bit fields
  - Masking to specific bit patterns
  - Clearing high bits

Implementation: src/vm/c32_vm.c line 612
```

#### 4.2.6 ORI - OR Immediate

```
Format:  ORI rt, rs, immediate
Opcode:  0x15
Type:    I-Type
Syntax:  rt = rs | imm

Description:
  Performs bitwise OR of rs and zero-extended immediate,
  stores result in rt.

Operation:
  R[rt] = R[rs] | imm

Exceptions: None

Example:
  ORI R5, R10, 0xF0  ; Set bits 4-7
  ; If R10 = 0x00000008, then R5 = 0x000000F8

Common Uses:
  - Setting specific bits
  - Loading small constants: ORI R5, R0, 42
  - Building addresses (with LUI)

Implementation: src/vm/c32_vm.c line 615
```

#### 4.2.7 XORI - XOR Immediate

```
Format:  XORI rt, rs, immediate
Opcode:  0x16
Type:    I-Type
Syntax:  rt = rs ^ imm

Description:
  Performs bitwise XOR of rs and zero-extended immediate,
  stores result in rt.

Operation:
  R[rt] = R[rs] ^ imm

Exceptions: None

Example:
  XORI R5, R10, 0xFF  ; Invert lower 8 bits

Common Uses:
  - Toggling specific bits
  - Simple encryption/obfuscation

Implementation: src/vm/c32_vm.c line 618
```

#### 4.2.8 LUI - Load Upper Immediate

```
Format:  LUI rt, immediate
Opcode:  0x17
Type:    I-Type
Syntax:  rt = imm << 16

Description:
  Loads the immediate value into the upper 16 bits of rt,
  clearing the lower 16 bits to zero.

Operation:
  R[rt] = imm << 16

Exceptions: None

Example:
  LUI R5, 0x1234      ; R5 = 0x12340000

Common Pattern (building 32-bit constant):
  LUI  R5, 0x1234     ; R5 = 0x12340000
  ORI  R5, R5, 0x5678 ; R5 = 0x12345678

Usage Notes:
  - Essential for loading large constants
  - Often paired with ORI for full 32-bit values
  - Can be used alone for aligned addresses

Implementation: src/vm/c32_vm.c line 621
```

### 4.3 Shift Instructions

#### 4.3.1 SLL - Shift Left Logical

```
Format:  SLL rd, rt, shift_amount
Opcode:  0x20
Type:    R-Type (uses immediate field for shift amount)
Syntax:  rd = rt << shift_amount

Description:
  Shifts rt left by shift_amount bits (0-31), filling with zeros.
  Stores result in rd.

Operation:
  R[rd] = R[rt] << (imm & 0x1F)

Exceptions: None

Example:
  SLL R2, R1, 2    ; R2 = R1 * 4
  ; If R1 = 8 (0x00000008), then R2 = 32 (0x00000020)

Bit Pattern:
  R1: 0000 0000 0000 0000 0000 0000 0000 1000
  R2: 0000 0000 0000 0000 0000 0000 0010 0000

Usage Notes:
  - Shift amount is masked to 5 bits (0-31)
  - Equivalent to multiply by 2^n
  - Upper bits shifted out are lost

Implementation: src/vm/c32_vm.c line 626
```

#### 4.3.2 SRL - Shift Right Logical

```
Format:  SRL rd, rt, shift_amount
Opcode:  0x21
Type:    R-Type (uses immediate field)
Syntax:  rd = rt >> shift_amount (logical)

Description:
  Shifts rt right by shift_amount bits (0-31), filling with zeros.
  Stores result in rd. Logical shift (no sign extension).

Operation:
  R[rd] = R[rt] >> (imm & 0x1F)

Exceptions: None

Example:
  SRL R3, R1, 1    ; R3 = R1 / 2 (unsigned)
  ; If R1 = 8 (0x00000008), then R3 = 4 (0x00000004)

For Negative Values:
  ; If R1 = 0x80000000 (-2147483648 as signed)
  SRL R3, R1, 1    ; R3 = 0x40000000 (not sign-extended!)

Implementation: src/vm/c32_vm.c line 629
```

#### 4.3.3 SRA - Shift Right Arithmetic

```
Format:  SRA rd, rt, shift_amount
Opcode:  0x22
Type:    R-Type (uses immediate field)
Syntax:  rd = rt >> shift_amount (arithmetic)

Description:
  Shifts rt right by shift_amount bits (0-31), preserving the sign bit.
  Fills with sign bit (bit 31). Stores result in rd.

Operation:
  R[rd] = (int32_t)R[rt] >> (imm & 0x1F)

Exceptions: None

Example:
  SRA R3, R1, 1    ; R3 = R1 / 2 (signed)
  ; If R1 = 8, then R3 = 4
  ; If R1 = -8 (0xFFFFFFF8), then R3 = -4 (0xFFFFFFFC)

Sign Extension:
  R1 (negative): 1111 1111 1111 1111 1111 1111 1111 1000
  R3 (>> 1):     1111 1111 1111 1111 1111 1111 1111 1100
                 ^--- sign bit preserved

Usage Notes:
  - Use for signed division by powers of 2
  - Rounds toward negative infinity

Implementation: src/vm/c32_vm.c line 632
```

#### 4.3.4 SLLV - Shift Left Logical Variable

```
Format:  SLLV rd, rt, rs
Opcode:  0x23
Type:    R-Type
Syntax:  rd = rt << rs

Description:
  Shifts rt left by the amount specified in the lower 5 bits of rs.
  Stores result in rd.

Operation:
  R[rd] = R[rt] << (R[rs] & 0x1F)

Exceptions: None

Example:
  SLLV R5, R10, R3
  ; If R10 = 8, R3 = 2, then R5 = 32

Usage Notes:
  - Shift amount comes from register, not immediate
  - Useful for variable shifts (e.g., bit field operations)
  - Only lower 5 bits of rs used (0-31)

Implementation: src/vm/c32_vm.c line 637
```

#### 4.3.5 SRLV - Shift Right Logical Variable

```
Format:  SRLV rd, rt, rs
Opcode:  0x24
Type:    R-Type
Syntax:  rd = rt >> rs (logical)

Description:
  Shifts rt right (logical) by the amount in lower 5 bits of rs.
  Stores result in rd.

Operation:
  R[rd] = R[rt] >> (R[rs] & 0x1F)

Exceptions: None

Example:
  SRLV R5, R10, R3
  ; If R10 = 32, R3 = 2, then R5 = 8

Implementation: src/vm/c32_vm.c line 640
```

#### 4.3.6 SRAV - Shift Right Arithmetic Variable

```
Format:  SRAV rd, rt, rs
Opcode:  0x25
Type:    R-Type
Syntax:  rd = rt >> rs (arithmetic)

Description:
  Shifts rt right (arithmetic) by the amount in lower 5 bits of rs.
  Preserves sign bit. Stores result in rd.

Operation:
  R[rd] = (int32_t)R[rt] >> (R[rs] & 0x1F)

Exceptions: None

Example:
  SRAV R5, R10, R3
  ; If R10 = -32, R3 = 2, then R5 = -8 (sign-extended)

Implementation: src/vm/c32_vm.c line 643
```

### 4.4 Comparison Instructions

#### 4.4.1 SLT - Set on Less Than (Signed)

```
Format:  SLT rd, rs, rt
Opcode:  0x30
Type:    R-Type
Syntax:  rd = (rs < rt) ? 1 : 0 (signed)

Description:
  Compares rs and rt as signed integers. Sets rd to 1 if rs < rt,
  otherwise sets rd to 0.

Operation:
  R[rd] = ((int32_t)R[rs] < (int32_t)R[rt]) ? 1 : 0

Exceptions: None

Example:
  SLT R3, R1, R2
  ; If R1 = 5, R2 = 10, then R3 = 1 (true)
  ; If R1 = 10, R2 = 5, then R3 = 0 (false)
  ; If R1 = -5, R2 = 5, then R3 = 1 (true, -5 < 5)

Common Pattern (conditional branches):
  SLT  R3, R1, R2   ; R3 = (R1 < R2)
  BNE  R3, R0, label ; Branch if R1 < R2

Implementation: src/vm/c32_vm.c line 648
```

#### 4.4.2 SLTU - Set on Less Than Unsigned

```
Format:  SLTU rd, rs, rt
Opcode:  0x31
Type:    R-Type
Syntax:  rd = (rs < rt) ? 1 : 0 (unsigned)

Description:
  Compares rs and rt as unsigned integers. Sets rd to 1 if rs < rt,
  otherwise sets rd to 0.

Operation:
  R[rd] = ((uint32_t)R[rs] < (uint32_t)R[rt]) ? 1 : 0

Exceptions: None

Example:
  SLTU R3, R1, R2
  ; If R1 = 5, R2 = 10, then R3 = 1 (5 < 10 unsigned)
  ; If R1 = 0xFFFFFFFF, R2 = 5, then R3 = 0
  ;   (4294967295 > 5 unsigned)

Usage Notes:
  - Essential for pointer comparisons
  - Use for unsigned arithmetic overflow detection

Implementation: src/vm/c32_vm.c line 651
```

#### 4.4.3 SLTI - Set on Less Than Immediate (Signed)

```
Format:  SLTI rt, rs, immediate
Opcode:  0x32
Type:    I-Type
Syntax:  rt = (rs < imm) ? 1 : 0 (signed)

Description:
  Compares rs with sign-extended immediate as signed integers.
  Sets rt to 1 if rs < imm, otherwise sets rt to 0.

Operation:
  R[rt] = ((int32_t)R[rs] < (int32_t)imm) ? 1 : 0

Exceptions: None

Example:
  SLTI R3, R1, 100
  ; If R1 = 50, then R3 = 1 (50 < 100)
  ; If R1 = 150, then R3 = 0 (150 >= 100)

Implementation: src/vm/c32_vm.c line 656
```

#### 4.4.4 SLTIU - Set on Less Than Immediate Unsigned

```
Format:  SLTIU rt, rs, immediate
Opcode:  0x33
Type:    I-Type
Syntax:  rt = (rs < imm) ? 1 : 0 (unsigned)

Description:
  Compares rs with zero-extended immediate as unsigned integers.
  Sets rt to 1 if rs < imm, otherwise sets rt to 0.

Operation:
  R[rt] = ((uint32_t)R[rs] < (uint32_t)imm) ? 1 : 0

Exceptions: None

Example:
  SLTIU R3, R1, 100
  ; If R1 = 50, then R3 = 1 (50 < 100 unsigned)

Implementation: src/vm/c32_vm.c line 659
```

### 4.5 Memory Access Instructions

All memory access instructions support virtual addressing when paging is enabled. Addresses are translated through the MMU before physical memory access.

**Translation Implementation:** `src/vm/c32_vm.c` lines 411-483

#### 4.5.1 LW - Load Word

```
Format:  LW rt, offset(rs)
Opcode:  0x50
Type:    M-Type
Syntax:  rt = mem[rs + offset]

Description:
  Loads a 32-bit word from memory at address (rs + offset),
  stores in rt. Address should be 4-byte aligned.

Operation:
  address = R[rs] + sign_extend(offset)
  R[rt] = mem[address:address+3]  (little-endian)

Exceptions:
  - Page fault (interrupt 8) if paging enabled and translation fails
  - MEM_FAULT (interrupt 2) if address out of bounds

Example:
  LW R8, 100(R2)
  ; Loads word from address (R2 + 100) into R8
  ; If R2 = 0x1000, loads from 0x1064

Alignment:
  - Recommended: 4-byte aligned
  - Unaligned access behavior is implementation-defined

Memory Layout (little-endian):
  Address 0x1064:  [byte0][byte1][byte2][byte3]
  R8:              0xbyte3byte2byte1byte0

Implementation: src/vm/c32_vm.c lines 735-742
```

#### 4.5.2 LH - Load Halfword Signed

```
Format:  LH rt, offset(rs)
Opcode:  0x51
Type:    M-Type
Syntax:  rt = sign_extend(mem[rs + offset])

Description:
  Loads a 16-bit halfword from memory, sign-extends to 32 bits,
  stores in rt. Address should be 2-byte aligned.

Operation:
  address = R[rs] + sign_extend(offset)
  halfword = mem[address:address+1]  (little-endian)
  R[rt] = sign_extend(halfword)

Exceptions:
  - Page fault if paging enabled
  - MEM_FAULT if address out of bounds

Example:
  LH R5, 10(R2)
  ; If mem[R2+10] = 0x8000 (16-bit)
  ; Then R5 = 0xFFFF8000 (sign-extended to -32768)

Sign Extension:
  Halfword 0x8000 (bit 15 = 1):
    Before: 1000 0000 0000 0000
    After:  1111 1111 1111 1111 1000 0000 0000 0000 (0xFFFF8000)

  Halfword 0x7FFF (bit 15 = 0):
    Before: 0111 1111 1111 1111
    After:  0000 0000 0000 0000 0111 1111 1111 1111 (0x00007FFF)

Implementation: src/vm/c32_vm.c lines 743-751
```

#### 4.5.3 LHU - Load Halfword Unsigned

```
Format:  LHU rt, offset(rs)
Opcode:  0x52
Type:    M-Type
Syntax:  rt = zero_extend(mem[rs + offset])

Description:
  Loads a 16-bit halfword from memory, zero-extends to 32 bits,
  stores in rt. Address should be 2-byte aligned.

Operation:
  address = R[rs] + sign_extend(offset)
  halfword = mem[address:address+1]
  R[rt] = zero_extend(halfword)

Exceptions:
  - Page fault if paging enabled
  - MEM_FAULT if address out of bounds

Example:
  LHU R5, 10(R2)
  ; If mem[R2+10] = 0x8000 (16-bit)
  ; Then R5 = 0x00008000 (zero-extended to 32768)

Zero Extension:
  Halfword 0x8000:
    Before: 1000 0000 0000 0000
    After:  0000 0000 0000 0000 1000 0000 0000 0000 (0x00008000)

Implementation: src/vm/c32_vm.c lines 752-759
```

#### 4.5.4 LB - Load Byte Signed

```
Format:  LB rt, offset(rs)
Opcode:  0x53
Type:    M-Type
Syntax:  rt = sign_extend(mem[rs + offset])

Description:
  Loads an 8-bit byte from memory, sign-extends to 32 bits,
  stores in rt. No alignment required.

Operation:
  address = R[rs] + sign_extend(offset)
  byte = mem[address]
  R[rt] = sign_extend(byte)

Exceptions:
  - Page fault if paging enabled
  - MEM_FAULT if address out of bounds

Example:
  LB R5, 5(R2)
  ; If mem[R2+5] = 0xFF (byte)
  ; Then R5 = 0xFFFFFFFF (sign-extended to -1)

Sign Extension:
  Byte 0x80 (bit 7 = 1):
    Before: 1000 0000
    After:  1111 1111 1111 1111 1111 1111 1000 0000 (0xFFFFFF80 = -128)

  Byte 0x7F (bit 7 = 0):
    Before: 0111 1111
    After:  0000 0000 0000 0000 0000 0000 0111 1111 (0x0000007F = 127)

Implementation: src/vm/c32_vm.c lines 760-768
```

#### 4.5.5 LBU - Load Byte Unsigned

```
Format:  LBU rt, offset(rs)
Opcode:  0x54
Type:    M-Type
Syntax:  rt = zero_extend(mem[rs + offset])

Description:
  Loads an 8-bit byte from memory, zero-extends to 32 bits,
  stores in rt. No alignment required.

Operation:
  address = R[rs] + sign_extend(offset)
  byte = mem[address]
  R[rt] = zero_extend(byte)

Exceptions:
  - Page fault if paging enabled
  - MEM_FAULT if address out of bounds

Example:
  LBU R5, 5(R2)
  ; If mem[R2+5] = 0xFF (byte)
  ; Then R5 = 0x000000FF (zero-extended to 255)

Common Use:
  ; String character loading
  LBU  R5, 0(R10)    ; Load character
  ADDI R10, R10, 1   ; Advance pointer
  BEQ  R5, R0, done  ; Check for null terminator

Implementation: src/vm/c32_vm.c lines 769-776
```

#### 4.5.6 SW - Store Word

```
Format:  SW rt, offset(rs)
Opcode:  0x58
Type:    M-Type
Syntax:  mem[rs + offset] = rt

Description:
  Stores the 32-bit value in rt to memory at address (rs + offset).
  Address should be 4-byte aligned.

Operation:
  address = R[rs] + sign_extend(offset)
  mem[address:address+3] = R[rt]  (little-endian)

Exceptions:
  - Page fault (interrupt 8) if:
    - Paging enabled and page not writable (W bit = 0)
    - Page not user-accessible in user mode
    - Invalid page
  - MEM_FAULT (interrupt 2) if address out of bounds

Example:
  SW R8, 100(R2)
  ; Stores R8 to address (R2 + 100)
  ; If R2 = 0x1000, R8 = 0x12345678
  ; Stores to 0x1064:
  ;   0x1064: 0x78
  ;   0x1065: 0x56
  ;   0x1066: 0x34
  ;   0x1067: 0x12

Implementation: src/vm/c32_vm.c lines 779-786
```

#### 4.5.7 SH - Store Halfword

```
Format:  SH rt, offset(rs)
Opcode:  0x59
Type:    M-Type
Syntax:  mem[rs + offset] = rt[15:0]

Description:
  Stores the lower 16 bits of rt to memory at address (rs + offset).
  Upper 16 bits of rt are ignored. Address should be 2-byte aligned.

Operation:
  address = R[rs] + sign_extend(offset)
  mem[address:address+1] = R[rt][15:0]  (little-endian)

Exceptions:
  - Page fault if paging enabled and page not writable
  - MEM_FAULT if address out of bounds

Example:
  SH R8, 10(R2)
  ; If R8 = 0x12345678, stores 0x5678 to (R2 + 10)
  ; Memory layout:
  ;   [R2+10]: 0x78
  ;   [R2+11]: 0x56

Implementation: src/vm/c32_vm.c lines 787-794
```

#### 4.5.8 SB - Store Byte

```
Format:  SB rt, offset(rs)
Opcode:  0x5A
Type:    M-Type
Syntax:  mem[rs + offset] = rt[7:0]

Description:
  Stores the lower 8 bits of rt to memory at address (rs + offset).
  Upper 24 bits of rt are ignored. No alignment required.

Operation:
  address = R[rs] + sign_extend(offset)
  mem[address] = R[rt][7:0]

Exceptions:
  - Page fault if paging enabled and page not writable
  - MEM_FAULT if address out of bounds

Example:
  SB R8, 5(R2)
  ; If R8 = 0x12345678, stores 0x78 to (R2 + 5)

Common Use:
  ; String character storing
  SB   R5, 0(R10)    ; Store character
  ADDI R10, R10, 1   ; Advance pointer

Implementation: src/vm/c32_vm.c lines 795-802
```

### 4.6 Branch Instructions

All branches are **PC-relative** with **byte offsets**. Branches have **no delay slots** - they take effect immediately.

**Branch Calculation:**
```
PC is advanced by 8 before offset is applied:
  new_PC = (current_PC + 8) + offset

To branch backward to current instruction:
  offset = -8

To branch forward by N instructions:
  offset = N * 8
```

#### 4.6.1 BEQ - Branch if Equal

```
Format:  BEQ rs, rt, offset
Opcode:  0x60
Type:    B-Type
Syntax:  if (rs == rt) PC = PC + offset

Description:
  Compares rs and rt. If equal, adds signed offset to PC.
  Otherwise, execution continues with next instruction.

Operation:
  if (R[rs] == R[rt])
    PC = PC + sign_extend(offset)
  ; Note: PC already advanced by 8

Exceptions: None

Example:
  ; Loop until R5 equals R6
  loop:
    ADDI R5, R5, 1        ; R5++
    BEQ  R5, R6, done     ; if R5 == R6, exit loop
    J    loop             ; continue loop
  done:

Unconditional Branch:
  BEQ R0, R0, target  ; Always branches (R0 always equals R0)

Implementation: src/vm/c32_vm.c lines 805-809
```

#### 4.6.2 BNE - Branch if Not Equal

```
Format:  BNE rs, rt, offset
Opcode:  0x61
Type:    B-Type
Syntax:  if (rs != rt) PC = PC + offset

Description:
  Compares rs and rt. If not equal, adds signed offset to PC.

Operation:
  if (R[rs] != R[rt])
    PC = PC + sign_extend(offset)

Exceptions: None

Example:
  ; Loop while R5 is not zero
  loop:
    ; ... loop body ...
    ADDI R5, R5, -1       ; R5--
    BNE  R5, R0, loop     ; Continue if R5 != 0

Implementation: src/vm/c32_vm.c lines 810-814
```

#### 4.6.3 BLEZ - Branch if Less Than or Equal to Zero

```
Format:  BLEZ rs, offset
Opcode:  0x62
Type:    B-Type
Syntax:  if (rs <= 0) PC = PC + offset

Description:
  Treats rs as signed integer. Branches if rs <= 0.
  rt field is ignored.

Operation:
  if ((int32_t)R[rs] <= 0)
    PC = PC + sign_extend(offset)

Exceptions: None

Example:
  ; Countdown loop
  ADDI R5, R0, 10       ; R5 = 10
  loop:
    ; ... loop body ...
    ADDI R5, R5, -1     ; R5--
    BGTZ R5, loop       ; Continue if R5 > 0
    ; Exits when R5 <= 0

Implementation: src/vm/c32_vm.c lines 815-819
```

#### 4.6.4 BGTZ - Branch if Greater Than Zero

```
Format:  BGTZ rs, offset
Opcode:  0x63
Type:    B-Type
Syntax:  if (rs > 0) PC = PC + offset

Description:
  Treats rs as signed integer. Branches if rs > 0.

Operation:
  if ((int32_t)R[rs] > 0)
    PC = PC + sign_extend(offset)

Exceptions: None

Example:
  BGTZ R5, positive_case
  ; Falls through if R5 <= 0

Implementation: src/vm/c32_vm.c lines 820-824
```

#### 4.6.5 BLTZ - Branch if Less Than Zero

```
Format:  BLTZ rs, offset
Opcode:  0x64
Type:    B-Type
Syntax:  if (rs < 0) PC = PC + offset

Description:
  Treats rs as signed integer. Branches if rs < 0 (negative).

Operation:
  if ((int32_t)R[rs] < 0)
    PC = PC + sign_extend(offset)

Exceptions: None

Example:
  ; Check if signed value is negative
  BLTZ R10, negative_handler
  ; Falls through if R10 >= 0

Sign Bit Check:
  ; BLTZ effectively checks bit 31
  ; Branches if bit 31 = 1

Implementation: src/vm/c32_vm.c lines 825-829
```

#### 4.6.6 BGEZ - Branch if Greater Than or Equal to Zero

```
Format:  BGEZ rs, offset
Opcode:  0x65
Type:    B-Type
Syntax:  if (rs >= 0) PC = PC + offset

Description:
  Treats rs as signed integer. Branches if rs >= 0 (non-negative).

Operation:
  if ((int32_t)R[rs] >= 0)
    PC = PC + sign_extend(offset)

Exceptions: None

Example:
  ; Check if signed value is non-negative
  BGEZ R10, non_negative_handler
  ; Falls through if R10 < 0

Unconditional Branch with Link:
  ; BAL (Branch and Link) pseudo-instruction:
  BGEZ R0, target  ; R0 always >= 0, so always branches
  ; If assembler provides BAL, it uses this encoding

Implementation: src/vm/c32_vm.c lines 830-834
```

### 4.7 Jump Instructions

Jump instructions provide unconditional control transfer. Unlike branches, jumps use **absolute addresses** (except JR/JALR which use register values).

#### 4.7.1 J - Jump

```
Format:  J target
Opcode:  0x70
Type:    J-Type
Syntax:  PC = target

Description:
  Unconditionally jumps to the 32-bit absolute address in the
  immediate field.

Operation:
  PC = imm

Exceptions: None

Example:
  J 0x1000      ; Jump to address 0x1000

Usage Notes:
  - Target address must be 8-byte aligned
  - Can jump anywhere in 4GB address space
  - No return address saved (use JAL for calls)

Infinite Loop:
  loop:
    ; ... some code ...
    J loop      ; Jump back to loop

Implementation: src/vm/c32_vm.c lines 837-839
```

#### 4.7.2 JAL - Jump and Link

```
Format:  JAL target
Opcode:  0x71
Type:    J-Type
Syntax:  R31 = PC; PC = target

Description:
  Saves the return address (PC after this instruction) in R31,
  then jumps to the target address. Used for function calls.

Operation:
  R[31] = PC  ; PC already advanced by 8
  PC = imm

Exceptions: None

Example:
  JAL function_name  ; Call function
  ; ... code continues here after function returns ...

function_name:
  ; ... function body ...
  JR R31  ; Return to caller

Return Address:
  ; PC is advanced by 8 before JAL executes
  ; So R31 points to the instruction after JAL
  ; At 0x1000: JAL 0x2000
  ; R31 = 0x1008 (next instruction)

Implementation: src/vm/c32_vm.c lines 840-843
```

#### 4.7.3 JR - Jump Register

```
Format:  JR rs
Opcode:  0x72
Type:    J-Type
Syntax:  PC = rs

Description:
  Unconditionally jumps to the address contained in register rs.
  Commonly used for function returns.

Operation:
  PC = R[rs]

Exceptions: None

Example:
  JR R31        ; Return from function (jump to return address)

Function Return Pattern:
  function:
    ; ... function body ...
    JR R31    ; Return to caller

Computed Jumps:
  ; Jump table implementation
  SLL  R5, R10, 2      ; R5 = index * 4
  LW   R6, table(R5)   ; Load jump target from table
  JR   R6              ; Jump to target

Implementation: src/vm/c32_vm.c lines 844-846
```

#### 4.7.4 JALR - Jump and Link Register

```
Format:  JALR rd, rs
Opcode:  0x73
Type:    J-Type
Syntax:  rd = PC; PC = rs

Description:
  Saves the return address in register rd, then jumps to the
  address in register rs. Allows non-R31 link registers.

Operation:
  R[rd] = PC  ; PC already advanced by 8
  PC = R[rs]

Exceptions: None

Example:
  JALR R5, R10  ; Save return address in R5, jump to address in R10

Common Usage:
  ; Function pointers
  JALR R31, R10  ; Call function pointer in R10
  ; Returns to address in R31

  ; Non-standard return register
  JALR R20, R15  ; Save return in R20, jump to R15

Position-Independent Code:
  ; Call relative function
  JAL get_pc
get_pc:
  ; R31 now contains current PC
  ADDI R10, R31, offset
  JALR R31, R10

Implementation: src/vm/c32_vm.c lines 847-850
```

### 4.8 System Instructions

#### 4.8.1 NOP - No Operation

```
Format:  NOP
Opcode:  0x00
Type:    Special
Syntax:  (no operation)

Description:
  Does nothing. PC advances by 8. All fields ignored.

Operation:
  ; No state changes (except PC += 8)

Exceptions: None

Example:
  NOP           ; Placeholder, timing delay

Encoding:
  00 00 00 00 00 00 00 00

Pseudo-Instruction Alternatives:
  OR  R0, R0, R0    ; Also a NOP
  ADD R0, R0, R0    ; Also a NOP

Usage:
  - Placeholder for future code
  - Alignment padding
  - Timing delays (in hardware implementations)
  - Code patching space

Implementation: src/vm/c32_vm.c line 572
```

#### 4.8.2 SYSCALL - System Call

```
Format:  SYSCALL
Opcode:  0xF0
Type:    System
Syntax:  Raise interrupt 4

Description:
  Raises interrupt 4 (SYSCALL) and halts the VM. Used for
  system calls or to return control to the host.

Operation:
  c32_raise_interrupt(vm, 4)
  vm->running = 0

Exceptions:
  - Interrupt 4 (SYSCALL)

Example:
  ; Exit program
  ADDI R4, R0, 1    ; System call number
  SYSCALL           ; Call OS

  ; System call with arguments
  ADDI R4, R0, 10   ; Function code
  ADDI R5, R0, 42   ; Argument 1
  SYSCALL

Interrupt Handler:
  ; OS installs handler at IVT[4]
  ; Handler reads R4 for syscall number
  ; Reads R5-R7 for arguments
  ; Returns result in R2
  ; Executes IRET to return

Implementation: src/vm/c32_vm.c lines 853-856
```

#### 4.8.3 BREAK - Breakpoint

```
Format:  BREAK
Opcode:  0xF1
Type:    System
Syntax:  Raise interrupt 5

Description:
  Raises interrupt 5 (BREAK) and halts the VM. Used for
  debugging breakpoints.

Operation:
  c32_raise_interrupt(vm, 5)
  vm->running = 0

Exceptions:
  - Interrupt 5 (BREAK)

Example:
  ; Debugger breakpoint
  BREAK

Debugger Usage:
  ; Debugger replaces instruction with BREAK
  ; Original: ADD R5, R10, R12
  ; Replace:  BREAK
  ; Handler examines state, restores original instruction

Implementation: src/vm/c32_vm.c lines 857-860
```

### 4.9 Interrupt Control Instructions

All interrupt control instructions are **privileged** - they can only execute in kernel mode. Attempting to execute them in user mode raises interrupt 7 (PRIVILEGE_VIOLATION).

**Privilege Check:** `src/vm/c32_vm.c` lines 864, 871, 878, 905, 912, 919, 927

#### 4.9.1 EI - Enable Interrupts

```
Format:  EI
Opcode:  0xF2
Type:    Privileged
Syntax:  interrupts.enabled = 1

Description:
  Sets the global interrupt enable flag, allowing interrupts
  to be processed. Privileged instruction (kernel mode only).

Operation:
  if (!kernel_mode)
    raise_interrupt(7)  ; PRIVILEGE_VIOLATION
  else
    vm->interrupts.enabled = 1

Exceptions:
  - Interrupt 7 (PRIVILEGE_VIOLATION) if in user mode

Example:
  ; Kernel initialization
  ; ... setup interrupt handlers ...
  EI          ; Enable interrupts

Typical Sequence:
  ; Setup IVT entries
  ; ... initialize handlers ...
  EI          ; Enable interrupts
  ; Interrupts can now fire

Implementation: src/vm/c32_vm.c lines 863-869
```

#### 4.9.2 DI - Disable Interrupts

```
Format:  DI
Opcode:  0xF3
Type:    Privileged
Syntax:  interrupts.enabled = 0

Description:
  Clears the global interrupt enable flag, preventing interrupts
  from being processed. Privileged instruction (kernel mode only).

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    vm->interrupts.enabled = 0

Exceptions:
  - Interrupt 7 if in user mode

Example:
  ; Critical section
  DI          ; Disable interrupts
  ; ... critical code ...
  EI          ; Re-enable interrupts

Usage Notes:
  - Interrupts are automatically disabled when entering an
    interrupt handler
  - Must explicitly re-enable with EI if needed
  - Nested interrupts require EI in handler

Implementation: src/vm/c32_vm.c lines 870-876
```

#### 4.9.3 IRET - Interrupt Return

```
Format:  IRET
Opcode:  0xF4
Type:    Privileged
Syntax:  Restore context from interrupt

Description:
  Returns from an interrupt handler. Restores all registers
  (R0-R31) from the stack, restores PC, and re-enables interrupts.
  Privileged instruction (kernel mode only).

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    ; Restore all registers R0-R31 from saved_regs_addr
    for i = 0 to 31:
      R[i] = mem[saved_regs_addr + i*4]
    ; Restore PC
    PC = saved_pc
    ; Enable interrupts
    interrupts.enabled = 1

Exceptions:
  - Interrupt 7 if in user mode

Example:
  ; Interrupt handler
  timer_handler:
    ; Registers already saved automatically
    ; ... handle interrupt ...
    IRET        ; Restore context and return

Context Restoration:
  - All 32 registers restored from stack
  - Stack pointer (R29) restored to pre-interrupt value
  - PC restored to saved_pc (instruction after interrupt)
  - Interrupts enabled (allowing nested interrupts)

Implementation: src/vm/c32_vm.c lines 877-895
```

#### 4.9.4 RAISE - Raise Software Interrupt

```
Format:  RAISE int_num
Opcode:  0xF5
Type:    System
Syntax:  Set pending bit for interrupt int_num

Description:
  Sets the pending bit for the specified interrupt number.
  The interrupt will be dispatched before the next instruction
  if interrupts are enabled. Not privileged.

Operation:
  pending[int_num / 8] |= (1 << (int_num % 8))

Exceptions: None

Example:
  RAISE 16    ; Raise timer interrupt

Usage:
  ; Inter-task communication
  RAISE 32    ; Signal task switch

  ; Deferred work
  RAISE 100   ; Schedule deferred processing

  ; Software exceptions
  RAISE 50    ; Raise custom exception

Implementation: src/vm/c32_vm.c lines 896-898
```

#### 4.9.5 GETPC - Get Saved PC

```
Format:  GETPC rd
Opcode:  0xF6
Type:    System
Syntax:  rd = saved_pc

Description:
  Loads the saved PC value (from interrupt context) into rd.
  Useful in interrupt handlers to determine the interrupted PC.

Operation:
  R[rd] = vm->interrupts.saved_pc

Exceptions: None

Example:
  ; In interrupt handler
  GETPC R5        ; R5 = address of interrupted instruction
  ; Can examine instruction at R5 to determine cause

Page Fault Handler Usage:
  page_fault_handler:
    GETPC R5      ; Get faulting PC
    ; Decode instruction at R5 to get faulting address
    ; Fix page table entry
    IRET          ; Retry instruction

Implementation: src/vm/c32_vm.c lines 899-901
```

### 4.10 Privilege and MMU Control Instructions

These instructions manage the privilege level and memory management unit (MMU). All are **privileged** except GETMODE.

#### 4.10.1 ENABLE_PAGING - Enable Virtual Addressing

```
Format:  ENABLE_PAGING
Opcode:  0xF7
Type:    Privileged
Syntax:  paging_enabled = 1

Description:
  Enables virtual address translation through the MMU. After this
  instruction, user mode memory accesses are translated through the
  page table. Kernel mode accesses bypass paging. Privileged.

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    vm->paging_enabled = 1

Exceptions:
  - Interrupt 7 (PRIVILEGE_VIOLATION) if in user mode

Example:
  ; Kernel setup sequence
  SET_PTBR R5, R6      ; Set page table base and size
  ; ... initialize page table entries ...
  ENABLE_PAGING        ; Turn on paging
  ENTER_USER           ; Drop to user mode

Prerequisites:
  - Page table must be set up with SET_PTBR first
  - Page table entries must be initialized
  - IVT entry 8 (page fault handler) should be installed

Implementation: src/vm/c32_vm.c lines 904-909
```

#### 4.10.2 DISABLE_PAGING - Disable Virtual Addressing

```
Format:  DISABLE_PAGING
Opcode:  0xF8
Type:    Privileged
Syntax:  paging_enabled = 0

Description:
  Disables virtual address translation. All memory accesses
  use physical addresses. Privileged instruction.

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    vm->paging_enabled = 0

Exceptions:
  - Interrupt 7 if in user mode

Example:
  ; Emergency shutdown
  DISABLE_PAGING    ; Disable MMU
  ; ... cleanup ...

Usage Notes:
  - Typically not used during normal operation
  - May be used for debugging or kernel panic handling
  - Kernel mode always uses physical addresses regardless

Implementation: src/vm/c32_vm.c lines 911-916
```

#### 4.10.3 SET_PTBR - Set Page Table Base Register

```
Format:  SET_PTBR base, num_pages
Opcode:  0xF9
Type:    Privileged
Syntax:  page_table_base = rd; num_pages = rt

Description:
  Sets the physical base address of the page table and the
  number of virtual pages. The page table must be in physical
  memory. Privileged instruction.

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    vm->page_table_base = R[rd]
    vm->num_pages = R[rt]

Exceptions:
  - Interrupt 7 if in user mode

Example:
  ; Setup 4MB virtual address space (1024 pages)
  LUI  R5, 0
  ORI  R5, R5, 0x9000     ; R5 = 0x9000 (page table address)
  ADDI R6, R0, 1024       ; R6 = 1024 pages
  SET_PTBR R5, R6         ; Set page table base and size

Page Table Structure:
  - Located at physical address 'base'
  - Contains 'num_pages' entries
  - Each entry is 4 bytes
  - Entry N at address: base + (N * 4)
  - Total size: num_pages * 4 bytes

Example Layout:
  base = 0x9000, num_pages = 1024:
    0x9000: PTE for page 0 (virtual 0x00000000-0x00000FFF)
    0x9004: PTE for page 1 (virtual 0x00001000-0x00001FFF)
    0x9008: PTE for page 2 (virtual 0x00002000-0x00002FFF)
    ...
    0x9FFC: PTE for page 1023 (virtual 0x003FF000-0x003FFFFF)

Implementation: src/vm/c32_vm.c lines 918-925
```

#### 4.10.4 ENTER_USER - Enter User Mode

```
Format:  ENTER_USER
Opcode:  0xFB
Type:    Privileged
Syntax:  kernel_mode = 0

Description:
  Drops privilege level from kernel mode to user mode. This is a
  one-way transition (cannot return to kernel without interrupt).
  Privileged instruction.

Operation:
  if (!kernel_mode)
    raise_interrupt(7)
  else
    vm->kernel_mode = 0

Exceptions:
  - Interrupt 7 if already in user mode (redundant but safe)

Example:
  ; Kernel launching user program
  kernel_start:
    ; ... initialize system ...
    ; ... setup user page table ...
    ENABLE_PAGING

    ; Setup user stack
    LUI  R29, 0x0040      ; User stack at 0x400000

    ; Jump to user code and drop privilege
    ENTER_USER
    J    user_main        ; Now in user mode

  user_main:
    ; User code runs here
    ; Cannot execute privileged instructions
    ; Memory access restricted by page table

Return to Kernel Mode:
  ; Only way back to kernel is through interrupt:
  SYSCALL         ; Raises interrupt, switches to kernel
  ; Interrupt handler runs in kernel mode

Implementation: src/vm/c32_vm.c lines 926-932
```

#### 4.10.5 GETMODE - Get Current Mode

```
Format:  GETMODE rd
Opcode:  0xFC
Type:    System (not privileged)
Syntax:  rd = kernel_mode

Description:
  Loads the current privilege level into rd. Returns 1 if in
  kernel mode, 0 if in user mode. Not privileged.

Operation:
  R[rd] = vm->kernel_mode

Exceptions: None

Example:
  GETMODE R5
  ; R5 = 1 if kernel mode
  ; R5 = 0 if user mode

  BNE R5, R0, kernel_path
  ; user mode code
  J done
kernel_path:
  ; kernel mode code
done:

Usage:
  - Runtime privilege checking
  - Adaptive code paths
  - Security verification

Implementation: src/vm/c32_vm.c lines 933-935
```

---

## 5. Memory Architecture

### 5.1 Physical Memory

CRISP-32 provides a flat 32-bit physical address space with byte-level addressability.

**Address Space:**
- **Range**: 0x00000000 to 0xFFFFFFFF (4 GB maximum)
- **Actual Size**: Determined by VM initialization
- **Access**: Direct for kernel, translated for user (if paging enabled)

**Memory Initialization:**
```c
/* Host code initializing VM */
uint8_t memory[16*1024*1024];  /* 16 MB */
c32_vm_t vm;

c32_vm_init(&vm, memory, sizeof(memory));
```

**Implementation:** `src/vm/c32_vm.c` lines 194-220

### 5.2 Interrupt Vector Table (IVT)

The IVT is a critical data structure located at the beginning of physical memory.

**Location**: 0x00000000 (fixed, not relocatable)

**Structure:**
```
Each IVT entry is 8 bytes:
  Bytes 0-3: Handler address (little-endian 32-bit)
  Bytes 4-7: Reserved (for future use)

Entry N location: 0x00000000 + (N × 8)

Total IVT size: 256 entries × 8 bytes = 2048 bytes (2 KB)
```

**Entry Format:**
```
Interrupt 0:
  0x00000000: [Handler Address] [Reserved]
Interrupt 1:
  0x00000008: [Handler Address] [Reserved]
Interrupt 2:
  0x00000010: [Handler Address] [Reserved]
...
Interrupt 255:
  0x000007F8: [Handler Address] [Reserved]
```

**Setting Handlers:**
```assembly
; Assembly code setting handler for interrupt 16
LUI  R5, 0
ORI  R5, R5, 128        ; R5 = 128 (16 × 8)
LUI  R6, 0
ORI  R6, R6, 0x2000     ; R6 = handler address
SW   R6, 0(R5)          ; Write to IVT
```

```c
/* C code using helper function */
c32_set_interrupt_handler(&vm, 16, 0x2000);
```

**Implementation:**
- Helper function: `src/vm/c32_vm.c` lines 280-291
- Interrupt dispatch: `src/vm/c32_vm.c` lines 365-369

### 5.3 Memory Access Patterns

**Typical Memory Layout:**
```
0x00000000 ┌─────────────────────────────────┐
           │ Interrupt Vector Table (IVT)    │ 2 KB
0x00000800 ├─────────────────────────────────┤
           │ Reserved                        │ 2 KB
0x00001000 ├─────────────────────────────────┤
           │ Kernel Code                     │
           │ - Initialization                │
           │ - Interrupt handlers            │
           │ - System services               │
0x00005000 ├─────────────────────────────────┤
           │ Kernel Data                     │
           │ - Global variables              │
           │ - System structures             │
0x00008000 ├─────────────────────────────────┤
           │ Kernel Stack                    │
           │ (grows downward)                │
0x00009000 ├─────────────────────────────────┤
           │ Page Table (if paging used)     │
           │ - 1024 entries for 4MB          │
           │ - 4 bytes per entry             │
0x0000A000 ├─────────────────────────────────┤
           │ User Physical Pages             │
           │ (mapped by page table)          │
           │                                 │
           │ Free Memory                     │
           │                                 │
           │                                 │
0x00400000 ├─────────────────────────────────┤
           │ User Stack (high memory)        │
           │ (grows downward)                │
0x00FFFFFF └─────────────────────────────────┘ (16 MB system)
```

### 5.4 Stack Architecture

CRISP-32 uses a **software-managed stack** with the following conventions:

**Stack Register**: R29 (sp) by convention

**Growth Direction**: Downward (toward lower addresses)

**Stack Operations:**
```assembly
; Push R5 onto stack
ADDI R29, R29, -4     ; Allocate space
SW   R5, 0(R29)       ; Store value

; Pop R5 from stack
LW   R5, 0(R29)       ; Load value
ADDI R29, R29, 4      ; Deallocate space

; Function prologue (save R16-R23, R31)
ADDI R29, R29, -36    ; Allocate 9 words
SW   R16, 0(R29)
SW   R17, 4(R29)
SW   R18, 8(R29)
; ... save other registers ...
SW   R31, 32(R29)     ; Save return address

; Function epilogue (restore registers)
LW   R16, 0(R29)
LW   R17, 4(R29)
; ... restore other registers ...
LW   R31, 32(R29)
ADDI R29, R29, 36     ; Deallocate
JR   R31              ; Return
```

**Interrupt Stack Usage:**

When an interrupt occurs, the VM automatically:
1. Allocates 128 bytes on stack (R29 -= 128)
2. Saves all 32 registers (R0-R31) to stack
3. Stores save location in `saved_regs_addr`
4. Handler can use stack normally
5. IRET restores all registers and deallocates

**Implementation:** `src/vm/c32_vm.c` lines 346-357 (save), lines 885-890 (restore)

### 5.5 Alignment Rules

| Data Type | Alignment | Instruction | Consequence if Misaligned |
|-----------|-----------|-------------|---------------------------|
| Byte | None (1-byte) | LB, LBU, SB | Always aligned |
| Halfword | 2-byte | LH, LHU, SH | Implementation-defined |
| Word | 4-byte | LW, SW | Implementation-defined |
| Instruction | 8-byte | Fetch | MEM_FAULT exception |

**Current Implementation:**
- Byte access: Always works
- Halfword/Word: No enforcement (works but not guaranteed portable)
- Instruction: Strictly enforced

**Best Practice:**
```assembly
; Good: Aligned access
data:
    .align 4
    .word 0x12345678

    LW R5, data      ; Aligned to 4 bytes

; Bad: Potentially unaligned
    LB  R5, 0(R10)
    LB  R6, 1(R10)
    LB  R7, 2(R10)
    LB  R8, 3(R10)
    ; Assembling word from bytes - slow but safe

; Good: Use proper instruction
    LW  R5, 0(R10)   ; Fast, but requires alignment
```

---

## 6. Privilege Levels and Protection

CRISP-32 implements a two-level privilege system suitable for most embedded and OS development use cases.

### 6.1 Privilege Levels

**Two Modes:**
1. **Kernel Mode** (`kernel_mode = 1`)
   - Full access to all instructions
   - Can execute privileged instructions
   - Bypass virtual memory (always physical addressing)
   - Can access all memory

2. **User Mode** (`kernel_mode = 0`)
   - Cannot execute privileged instructions
   - Subject to memory protection (if paging enabled)
   - Isolated from kernel and other processes

**Privilege State Storage:**
- Stored in VM structure: `vm->kernel_mode`
- Not directly accessible to guest code
- Can be read with GETMODE instruction
- Cannot be directly set by user code

**Implementation:** `include/c32_vm.h` line 60

### 6.2 Privileged Instructions

The following instructions can only execute in kernel mode:

| Instruction | Opcode | Description |
|-------------|--------|-------------|
| EI | 0xF2 | Enable interrupts |
| DI | 0xF3 | Disable interrupts |
| IRET | 0xF4 | Return from interrupt |
| ENABLE_PAGING | 0xF7 | Enable virtual addressing |
| DISABLE_PAGING | 0xF8 | Disable virtual addressing |
| SET_PTBR | 0xF9 | Set page table base and size |
| ENTER_USER | 0xFB | Drop to user mode |

**Privilege Violation:**
- Attempting privileged instruction in user mode raises interrupt 7
- VM checks `kernel_mode` flag before execution
- Handler can log violation and terminate process

**Implementation:** Privilege checks at `src/vm/c32_vm.c` lines 864, 871, 878, 905, 912, 919, 927

### 6.3 Mode Transitions

#### 6.3.1 User → Kernel (Automatic)

Any interrupt automatically switches to kernel mode:

1. Hardware/software interrupt raised
2. VM sets `kernel_mode = 1`
3. Saves all registers (R0-R31) to stack
4. Saves current PC
5. Disables interrupts
6. Jumps to interrupt handler (in kernel mode)

**Triggers:**
- SYSCALL instruction (interrupt 4)
- BREAK instruction (interrupt 5)
- Page faults (interrupt 8)
- External interrupts
- Software interrupts (RAISE)
- Exceptions (illegal instruction, etc.)

**Implementation:** `src/vm/c32_vm.c` lines 342-344 (mode switch during interrupt)

#### 6.3.2 Kernel → User (Explicit)

**Method 1: ENTER_USER Instruction**
```assembly
; Kernel code
setup_user_task:
    ; Initialize user environment
    LUI  R29, 0x0040        ; User stack
    ; ... setup page table ...
    ENABLE_PAGING
    ENTER_USER              ; Drop to user mode
    J    user_main          ; Now in user mode
```

**Method 2: IRET (if interrupted user code)**
```assembly
; Interrupt handler
syscall_handler:
    ; Handle system call
    ; ... process request ...
    IRET                    ; Return to user mode
    ; Note: Restores privilege level from interrupt context
```

### 6.4 Protection Scenarios

#### Scenario 1: No Protection (Simple Embedded)

```c
/* VM initialization */
vm.kernel_mode = 1;
vm.paging_enabled = 0;

/* Everything runs in kernel mode with physical addressing */
/* Suitable for: */
/* - Simple firmware */
/* - Bootloaders */
/* - Single-task systems */
```

#### Scenario 2: Privilege Separation Only

```c
vm.kernel_mode = 0;        /* User mode */
vm.paging_enabled = 0;     /* No paging */

/* User cannot execute privileged instructions */
/* but has access to all physical memory */
/* Suitable for: */
/* - Cooperative multitasking */
/* - Untrusted code isolation (instructions only) */
```

#### Scenario 3: Full Memory Protection

```c
vm.kernel_mode = 0;        /* User mode */
vm.paging_enabled = 1;     /* Paging enabled */
vm.page_table_base = 0x9000;
vm.num_pages = 1024;

/* User has virtual addressing with permission checks */
/* Suitable for: */
/* - Multi-process OS */
/* - Sandboxing */
/* - Memory isolation */
```

### 6.5 Security Model

**Kernel Responsibilities:**
1. Setup interrupt vectors before enabling interrupts
2. Initialize page table before enabling paging
3. Validate user inputs in syscall handlers
4. Protect kernel memory from user access
5. Clean up on context switches

**User Code Restrictions:**
1. Cannot execute privileged instructions
2. Cannot access memory outside page table permissions
3. Cannot disable interrupts
4. Cannot modify page tables
5. Cannot directly change privilege level

**Attack Surface:**
- Syscall interface (validate all parameters)
- Page table setup (ensure kernel pages not user-accessible)
- Interrupt handlers (check caller privilege)

---

## 7. Virtual Memory and Paging

CRISP-32 provides optional virtual memory through a simple flat page table. The paging system allows memory protection and isolation when needed.

### 7.1 Virtual Memory Overview

**Page Size**: 4 KB (4096 bytes)

**Virtual Address Format:**
```
 31                                    12 11                    0
┌─────────────────────────────────────┬──────────────────────────┐
│       Page Number (20 bits)         │  Page Offset (12 bits)  │
│                                     │                          │
└─────────────────────────────────────┴──────────────────────────┘
  VPN (Virtual Page Number)              Offset within page
```

**Address Components:**
- **Page Number (VPN)**: Bits [31:12] - selects which 4KB page
- **Page Offset**: Bits [11:0] - byte offset within the page

**Address Calculation:**
```
Virtual Address = (Page Number << 12) | Page Offset

Example:
  Virtual Address 0x00003ABC
  Page Number = 0x00003 (bits [31:12])
  Page Offset = 0xABC (bits [11:0])
```

### 7.2 Page Table Structure

**Location**: Stored in guest physical memory

**Base Address**: Set with SET_PTBR instruction

**Entry Format**: Each entry is 32 bits (4 bytes)

**Entry Layout:**
```
 31                                12 11    4  3  2  1  0
┌─────────────────────────────────┬───────┬──┬──┬──┬──┐
│  Physical Page Number (20 bits) │Reserve│U │X │W │V │
│                                 │       │  │  │  │  │
└─────────────────────────────────┴───────┴──┴──┴──┴──┘
  PPN (Physical Page Number)         8     1  1  1  1
                                   bits
```

**Field Descriptions:**

| Field | Bits | Name | Description |
|-------|------|------|-------------|
| PPN | [31:12] | Physical Page Number | Upper 20 bits of physical address |
| Reserved | [11:4] | Reserved | Must be zero (reserved for future use) |
| U | [3] | User | 1 = accessible in user mode, 0 = kernel only |
| X | [2] | Executable | 1 = can execute code, 0 = no execute |
| W | [1] | Writable | 1 = can write, 0 = read-only |
| V | [0] | Valid | 1 = page present, 0 = page fault |

**Calculating Entry Address:**
```
Entry address = page_table_base + (VPN × 4)

Example:
  page_table_base = 0x9000
  VPN = 5
  Entry address = 0x9000 + (5 × 4) = 0x9014
```

**Implementation:** `src/vm/c32_vm.c` lines 411-483 (translation logic)

### 7.3 Address Translation Process

#### 7.3.1 Translation Algorithm

```
Input: Virtual address (vaddr)
Output: Physical address (paddr) or Page Fault

1. Check if kernel mode:
   if (kernel_mode)
     return vaddr  /* Kernel bypasses paging */

2. Check if paging disabled:
   if (!paging_enabled)
     return vaddr  /* Direct mapping */

3. Extract page number and offset:
   vpn = vaddr >> 12
   offset = vaddr & 0x0FFF

4. Check page number bounds:
   if (vpn >= num_pages)
     raise PAGE_FAULT  /* Out of bounds */

5. Read PTE from page table:
   pte_addr = page_table_base + (vpn * 4)
   pte = read_word(memory[pte_addr])

6. Extract PTE fields:
   ppn = pte & 0xFFFFF000
   u_bit = (pte >> 3) & 1
   x_bit = (pte >> 2) & 1
   w_bit = (pte >> 1) & 1
   v_bit = pte & 1

7. Check valid bit:
   if (!v_bit)
     raise PAGE_FAULT  /* Invalid page */

8. Check user accessible (in user mode):
   if (!u_bit)
     raise PAGE_FAULT  /* Not user accessible */

9. Check write permission (for stores):
   if (is_write && !w_bit)
     raise PAGE_FAULT  /* Write to read-only */

10. Check execute permission (for instruction fetch):
    if (is_exec && !x_bit)
      raise PAGE_FAULT  /* Execute on non-executable */

11. Construct physical address:
    paddr = ppn | offset

12. Return paddr
```

**Implementation:** `src/vm/c32_vm.c` lines 411-483

#### 7.3.2 Translation Example

**Setup:**
```
Virtual address:     0x00003ABC
page_table_base:     0x9000
num_pages:           1024
kernel_mode:         0 (user mode)
paging_enabled:      1

Page table entry at 0x9000 + (3 × 4) = 0x900C:
  PTE = 0x0000A00F
    PPN = 0x0000A000
    Reserved = 0
    U = 1 (user accessible)
    X = 1 (executable)
    W = 1 (writable)
    V = 1 (valid)
```

**Translation Steps:**
```
1. Extract VPN and offset:
   VPN = 0x00003ABC >> 12 = 0x00003
   Offset = 0x00003ABC & 0xFFF = 0xABC

2. Read PTE:
   PTE address = 0x9000 + (3 × 4) = 0x900C
   PTE = 0x0000A00F

3. Extract PPN:
   PPN = 0x0000A00F & 0xFFFFF000 = 0x0000A000

4. Check permissions:
   V = 1 ✓ (valid)
   U = 1 ✓ (user accessible)

5. Construct physical address:
   Physical = 0x0000A000 | 0xABC = 0x0000AABC

Result: Virtual 0x00003ABC → Physical 0x0000AABC
```

### 7.4 Page Table Setup

#### 7.4.1 Kernel Setup Example

```assembly
; Setup 4MB virtual address space (1024 pages of 4KB each)
; Page table at physical address 0x9000

setup_paging:
    ; 1. Set page table base and size
    LUI  R5, 0
    ORI  R5, R5, 0x9000      ; R5 = 0x9000 (page table base)
    ADDI R6, R0, 1024        ; R6 = 1024 (number of pages)
    SET_PTBR R5, R6

    ; 2. Map virtual page 0 to physical page 0xA000
    ;    with U|X|W|V permissions (0x0F)
    LUI  R7, 0
    ORI  R7, R7, 0x9000      ; R7 = page table base

    ; Build PTE: physical 0xA000 | flags 0x0F
    LUI  R8, 0
    ORI  R8, R8, 0xA000      ; Physical page number
    ORI  R8, R8, 0x0F        ; Set U|X|W|V

    ; Write PTE for page 0
    SW   R8, 0(R7)           ; page_table[0] = PTE

    ; 3. Map virtual page 1 to physical page 0xB000
    LUI  R8, 0
    ORI  R8, R8, 0xB000
    ORI  R8, R8, 0x0F
    SW   R8, 4(R7)           ; page_table[1] = PTE

    ; 4. Map virtual page 2 (read-only code page)
    LUI  R8, 0
    ORI  R8, R8, 0xC000
    ORI  R8, R8, 0x0D        ; U|X|V (no W - read-only)
    SW   R8, 8(R7)           ; page_table[2] = PTE

    ; 5. Enable paging
    ENABLE_PAGING

    ; 6. Drop to user mode
    ENTER_USER

    ; 7. Jump to user code (now uses virtual addressing)
    J    0x0000              ; Virtual address 0 (mapped to 0xA000)
```

#### 7.4.2 Identity Mapping

Identity mapping (virtual == physical) for kernel region:

```assembly
; Identity map 16MB (4096 pages) for kernel
setup_identity_map:
    LUI  R10, 0
    ORI  R10, R10, 0x9000    ; Page table base
    ADDI R11, R0, 0          ; Page counter

identity_loop:
    ; Build PTE: page number << 12 | 0x0F (U|X|W|V)
    SLL  R12, R11, 12        ; Physical address
    ORI  R12, R12, 0x0F      ; Set flags

    ; Write to page table
    SLL  R13, R11, 2         ; Entry offset = page * 4
    ADD  R13, R10, R13       ; Entry address
    SW   R12, 0(R13)         ; Store PTE

    ; Next page
    ADDI R11, R11, 1
    SLTI R14, R11, 4096      ; Check if done
    BNE  R14, R0, identity_loop

    ; Done - 4096 pages mapped
    JR   R31
```

### 7.5 Page Faults

When address translation fails, interrupt 8 (PAGE_FAULT) is raised.

**Page Fault Causes:**

1. **Invalid Page** (V bit = 0)
   - Page not present in memory
   - Page table entry not initialized

2. **Permission Violation**
   - User accessing kernel page (U bit = 0)
   - Write to read-only page (W bit = 0)
   - Execute on non-executable page (X bit = 0)

3. **Out of Bounds**
   - VPN >= num_pages

**Page Fault Handler Example:**

```assembly
page_fault_handler:
    ; Save additional context
    GETPC R26                ; Get faulting PC

    ; Determine fault type by examining faulting instruction
    ; (simplified - real handler would decode instruction)

    ; Option 1: Fix the page table entry
    ; ... load missing page from disk ...
    ; ... update PTE to mark valid ...

    ; Option 2: Kill the process
    ; ... log error ...
    ; ... cleanup ...
    ; ... don't return (or return to kill routine) ...

    ; Option 3: Retry
    IRET                     ; Retry faulting instruction
```

**Handling Strategy:**
```c
void page_fault_handler(c32_vm_t *vm) {
    uint32_t faulting_pc = vm->interrupts.saved_pc;

    /* Decode instruction at faulting_pc to get virtual address */
    /* ... */

    /* Determine cause */
    uint32_t vpn = vaddr >> 12;
    if (vpn >= vm->num_pages) {
        /* Out of bounds - kill process */
        return;
    }

    uint32_t pte_addr = vm->page_table_base + (vpn * 4);
    uint32_t pte = c32_read_word(vm->memory + pte_addr);

    if (!(pte & 1)) {
        /* Invalid page - load from disk/swap */
        /* ... load page ... */
        pte |= 1;  /* Mark valid */
        c32_write_word(vm->memory + pte_addr, pte);
        /* Return via IRET to retry */
    } else {
        /* Permission violation - kill process */
        /* ... */
    }
}
```

**Implementation:** Page fault raised at `src/vm/c32_vm.c` lines 432, 440, 456, 464, 470, 476

### 7.6 Memory Protection Patterns

#### Pattern 1: Code Segment (Read-Only, Executable)

```
PTE flags: U=1, X=1, W=0, V=1
Binary: 0x0D
```

```assembly
; Map code page at virtual 0x1000 to physical 0x20000
LUI  R8, 2              ; Physical base = 0x20000
ORI  R8, R8, 0x0D       ; U|X|V (no W)
LUI  R7, 0
ORI  R7, R7, 0x9004     ; PTE address for page 1
SW   R8, 0(R7)
```

**Effect:**
- User can read and execute code
- Cannot modify code (W=0)
- Attempts to write raise page fault

#### Pattern 2: Data Segment (Read-Write, No Execute)

```
PTE flags: U=1, X=0, W=1, V=1
Binary: 0x0B
```

```assembly
; Map data page at virtual 0x2000 to physical 0x21000
LUI  R8, 2
ORI  R8, R8, 0x1000
ORI  R8, R8, 0x0B       ; U|W|V (no X)
LUI  R7, 0
ORI  R7, R7, 0x9008     ; PTE for page 2
SW   R8, 0(R7)
```

**Effect:**
- User can read and write data
- Cannot execute code from this page
- NX (No Execute) protection

#### Pattern 3: Stack (Read-Write, No Execute)

```
Same as data segment: 0x0B (U|W|V)
```

#### Pattern 4: Kernel Page (Not User Accessible)

```
PTE flags: U=0, X=1, W=1, V=1
Binary: 0x07
```

```assembly
; Map kernel page (user cannot access)
LUI  R8, 1
ORI  R8, R8, 0x0000
ORI  R8, R8, 0x07       ; X|W|V (no U)
SW   R8, 0(R7)
```

**Effect:**
- Kernel can access (kernel bypasses paging)
- User gets page fault if attempted
- Protects kernel memory

### 7.7 Example: Complete Process Setup

```assembly
; Create isolated process with code/data/stack

create_process:
    ; Allocate 3 pages: code, data, stack
    ; Virtual layout:
    ;   0x00000000-0x00000FFF: Code (R-X)
    ;   0x00001000-0x00001FFF: Data (RW-)
    ;   0x00002000-0x00002FFF: Stack (RW-)

    ; Physical allocation:
    ;   Code at 0x100000
    ;   Data at 0x101000
    ;   Stack at 0x102000

    ; Page table at 0x9000

    ; Map code page (virtual 0 → physical 0x100000, R-X)
    LUI  R10, 0x0010        ; R10 = 0x100000
    ORI  R10, R10, 0x0D     ; U|X|V
    LUI  R11, 0
    ORI  R11, R11, 0x9000
    SW   R10, 0(R11)        ; page_table[0]

    ; Map data page (virtual 0x1000 → physical 0x101000, RW-)
    LUI  R10, 0x0010
    ORI  R10, R10, 0x1000
    ORI  R10, R10, 0x0B     ; U|W|V
    SW   R10, 4(R11)        ; page_table[1]

    ; Map stack page (virtual 0x2000 → physical 0x102000, RW-)
    LUI  R10, 0x0010
    ORI  R10, R10, 0x2000
    ORI  R10, R10, 0x0B     ; U|W|V
    SW   R10, 8(R11)        ; page_table[2]

    ; Set page table
    LUI  R5, 0
    ORI  R5, R5, 0x9000
    ADDI R6, R0, 3          ; 3 pages
    SET_PTBR R5, R6

    ; Enable paging
    ENABLE_PAGING

    ; Setup stack pointer (top of stack page)
    LUI  R29, 0
    ORI  R29, R29, 0x3000   ; Virtual 0x3000 (just past stack page)

    ; Enter user mode and start process
    ENTER_USER
    J    0x0000             ; Virtual address 0 (code page)
```

**Memory Map:**
```
Virtual Address Space (Process View):
┌────────────────────────────────────┐
│ 0x00000000  Code Page (4KB)        │ R-X
│             [Instructions]         │
├────────────────────────────────────┤
│ 0x00001000  Data Page (4KB)        │ RW-
│             [Global variables]     │
├────────────────────────────────────┤
│ 0x00002000  Stack Page (4KB)       │ RW-
│             [Stack grows down]     │
└────────────────────────────────────┘

Physical Memory:
┌────────────────────────────────────┐
│ 0x100000    Code (mapped)          │
├────────────────────────────────────┤
│ 0x101000    Data (mapped)          │
├────────────────────────────────────┤
│ 0x102000    Stack (mapped)         │
└────────────────────────────────────┘
```

---

## 8. Interrupt System

CRISP-32 provides a comprehensive interrupt system with 256 interrupt vectors, priority-based handling, and full context save/restore.

### 8.1 Interrupt Types

CRISP-32 supports 256 interrupt types (0-255), divided into the following categories:

**Standard Interrupts (0-15):**

| Number | Name | Description | Raised By |
|--------|------|-------------|-----------|
| 0 | RESET | System reset | External/power-on |
| 1 | ILLEGAL_OP | Illegal instruction | VM on unknown opcode |
| 2 | MEM_FAULT | Memory access fault | VM on bounds check |
| 3 | ALIGN_FAULT | Alignment fault | VM on misaligned PC |
| 4 | SYSCALL | System call | SYSCALL instruction |
| 5 | BREAK | Breakpoint | BREAK instruction |
| 6 | TIMER | Timer interrupt | External timer |
| 7 | PRIVILEGE_VIOLATION | Privilege violation | VM on privileged instruction in user mode |
| 8 | PAGE_FAULT | Page fault | MMU on translation failure |
| 9-15 | Reserved | Reserved for future use | - |

**Software Interrupts (16-127):**
- Available for application use
- Can be raised with RAISE instruction
- Used for inter-process communication, deferred work, etc.

**Hardware/External Interrupts (128-255):**
- Reserved for external devices
- Keyboard, disk, network, etc.
- Implementation-specific

**Implementation:** Exception types raised in `src/vm/c32_vm.c` lines 540, 854, 858, 865, 872, 879, 906, 913, 920, 928, 939

### 8.2 Interrupt Vector Table (IVT)

The IVT maps interrupt numbers to handler addresses. It is located at physical address 0x00000000.

**Structure:**
```
Location: Physical address 0x00000000
Size: 2048 bytes (256 entries × 8 bytes)

Entry format (8 bytes):
  Offset 0-3: Handler address (32-bit, little-endian)
  Offset 4-7: Reserved (for future use)

Entry address = 0x00000000 + (interrupt_number × 8)
```

**Example IVT Setup:**
```assembly
; Setup interrupt handlers

; Handler for interrupt 4 (SYSCALL)
LUI  R5, 0
ORI  R5, R5, 32         ; IVT entry 4 is at offset 32 (4 × 8)
LUI  R6, 0
ORI  R6, R6, 0x2000     ; Handler address
SW   R6, 0(R5)          ; Write handler address to IVT

; Handler for interrupt 8 (PAGE_FAULT)
LUI  R5, 0
ORI  R5, R5, 64         ; IVT entry 8 is at offset 64 (8 × 8)
LUI  R6, 0
ORI  R6, R6, 0x3000     ; Handler address
SW   R6, 0(R5)
```

**C Helper Function:**
```c
/* Set handler for interrupt 16 */
c32_set_interrupt_handler(&vm, 16, 0x4000);
```

**Implementation:** `src/vm/c32_vm.c` lines 280-291

### 8.3 Interrupt Handling Mechanism

#### 8.3.1 Interrupt Dispatch Sequence

When an interrupt fires, the following sequence occurs:

**1. Check Interrupt Enable**
- If `interrupts.enabled == 0`, interrupt is queued but not dispatched
- Interrupts are checked before each instruction execution

**2. Save Context (Automatic)**
```
1. Save current PC to saved_pc
2. Switch to kernel mode (kernel_mode = 1)
3. Allocate 128 bytes on stack (R29 -= 128)
4. Save all registers R0-R31 to stack
   - Address: saved_regs_addr = R29
   - Each register: 4 bytes
   - Total: 32 × 4 = 128 bytes
5. Disable interrupts (interrupts.enabled = 0)
6. Load interrupt number into R4
```

**3. Jump to Handler**
```
1. Read handler address from IVT
   - IVT entry = interrupt_number × 8
   - Handler address = read_word(IVT + entry)
2. Set PC = handler address
3. Begin executing handler
```

**Implementation:** `src/vm/c32_vm.c` lines 318-381 (check_interrupts function)

#### 8.3.2 Interrupt Handler Structure

Typical interrupt handler:

```assembly
; Generic interrupt handler
interrupt_handler:
    ; Context already saved by VM
    ; R4 contains interrupt number
    ; All registers preserved
    ; Running in kernel mode
    ; Interrupts disabled

    ; Handler can use any register freely
    ; R26-R27 (k0-k1) are kernel-reserved

    ; Check interrupt type
    ADDI R26, R0, 4
    BEQ  R4, R26, syscall_handler
    ADDI R26, R0, 8
    BEQ  R4, R26, page_fault_handler
    ; ... more handlers ...

    ; Unknown interrupt - just return
    IRET

syscall_handler:
    ; Handle system call
    ; R5-R7 contain arguments
    ; Return value in R2
    ; ... process syscall ...
    IRET

page_fault_handler:
    ; Handle page fault
    GETPC R26              ; Get faulting PC
    ; ... fix page table or kill process ...
    IRET
```

#### 8.3.3 Context Restoration (IRET)

The IRET instruction restores the interrupted context:

**IRET Sequence:**
```
1. Restore all registers R0-R31 from stack
   - Load from saved_regs_addr
   - 32 registers × 4 bytes each
2. Deallocate stack (R29 += 128, restored from stack)
3. Restore PC from saved_pc
4. Enable interrupts (interrupts.enabled = 1)
5. Resume execution at restored PC
```

**Note:** After IRET, the system returns to the privilege level it was in before the interrupt (user or kernel), because all registers including the state are restored.

**Implementation:** `src/vm/c32_vm.c` lines 877-895

### 8.4 Interrupt Priority

CRISP-32 uses a simple priority scheme:

**Priority Order:** Lower interrupt numbers have higher priority.
- Interrupt 0 (RESET) has highest priority
- Interrupt 255 has lowest priority
- When multiple interrupts are pending, lowest-numbered is dispatched first

**Priority Scanning:**
```c
/* From src/vm/c32_vm.c lines 329-338 */
for (int_num = 0; int_num < 255; int_num++) {
    if (pending[int_num / 8] & (1 << (int_num % 8))) {
        /* Dispatch this interrupt */
        break;
    }
}
```

**Implications:**
- Critical exceptions (ILLEGAL_OP, MEM_FAULT) handled before user interrupts
- System calls processed before software interrupts
- Hardware interrupts can preempt software processing

### 8.5 Nested Interrupts

By default, interrupts are **disabled** when entering an interrupt handler.

**Enabling Nested Interrupts:**

```assembly
interrupt_handler:
    ; Context saved, interrupts disabled

    ; Save critical registers if needed
    ADDI R29, R29, -8
    SW   R26, 0(R29)
    SW   R27, 4(R29)

    ; Enable interrupts for nesting
    EI

    ; Handler body
    ; Higher priority interrupts can now preempt
    ; ...

    ; Disable interrupts before cleanup
    DI

    ; Restore saved registers
    LW   R26, 0(R29)
    LW   R27, 4(R29)
    ADDI R29, R29, 8

    ; Return (re-enables interrupts)
    IRET
```

**Nested Interrupt Sequence:**
1. Interrupt A fires, handler A begins (interrupts disabled)
2. Handler A executes EI (enables interrupts)
3. Interrupt B fires (higher priority)
4. Context saved again (nested on stack)
5. Handler B executes
6. Handler B executes IRET
7. Returns to handler A
8. Handler A completes
9. Handler A executes IRET
10. Returns to original code

**Stack Layout During Nesting:**
```
High Address
┌─────────────────────┐
│ Original stack      │
├─────────────────────┤ ← R29 before interrupt A
│ Saved context A     │ 128 bytes (R0-R31)
├─────────────────────┤ ← R29 during handler A
│ Handler A locals    │
├─────────────────────┤ ← R29 when interrupt B fires
│ Saved context B     │ 128 bytes (R0-R31)
├─────────────────────┤ ← R29 during handler B
│ Handler B locals    │
└─────────────────────┘ ← Current R29
Low Address
```

### 8.6 Software Interrupts

Software interrupts allow user code to signal events or request services.

**Raising Software Interrupts:**

```assembly
; Raise interrupt 32 (custom event)
ADDI R5, R0, 32
; Put value in immediate field
; Note: RAISE uses immediate field for interrupt number
RAISE 32

; Alternative if interrupt number in register:
; (RAISE instruction takes immediate, so load to code)
```

**Software Interrupt Use Cases:**

1. **Inter-Process Communication**
```assembly
; Signal process switch
RAISE 16
```

2. **Deferred Work**
```assembly
; Schedule network processing
RAISE 100
```

3. **Custom Exceptions**
```assembly
; Raise division by zero exception
RAISE 50
```

4. **Debugging**
```assembly
; Trigger debug trap
RAISE 127
```

**Pending Interrupt Bitmap:**

The VM maintains a 256-bit bitmap (32 bytes) of pending interrupts:
```c
/* From include/c32_vm.h lines 76-77 */
uint8_t pending[32];  /* 256 bits */
```

Each bit represents one interrupt:
- Bit 0 of byte 0: Interrupt 0
- Bit 7 of byte 0: Interrupt 7
- Bit 0 of byte 1: Interrupt 8
- ...
- Bit 7 of byte 31: Interrupt 255

**Setting Pending Bit:**
```c
/* From src/vm/c32_vm.c lines 260-267 */
void c32_raise_interrupt(c32_vm_t *vm, uint8_t int_num) {
    uint8_t byte_idx = int_num / 8;
    uint8_t bit_idx = int_num % 8;
    vm->interrupts.pending[byte_idx] |= (1 << bit_idx);
}
```

### 8.7 Interrupt Latency

**Definition:** Time from interrupt raised to handler first instruction.

**Latency Components:**
1. Pending check (before each instruction)
2. Context save (128 bytes written)
3. IVT read (1 word)
4. PC update
5. First instruction fetch

**Worst Case:**
- Instruction just started executing
- Must complete current instruction
- Then service interrupt

**Best Case:**
- Interrupt raised between instructions
- Immediate dispatch

**Typical Latency (VM implementation):**
- ~100-200 host CPU cycles
- Depends on host memory bandwidth
- Context save dominates (32 register writes)

**Reducing Latency:**
- Keep handlers short
- Use deferred processing for long tasks
- Enable nested interrupts for critical handlers

### 8.8 Interrupt Handler Examples

#### Example 1: System Call Handler

```assembly
; SYSCALL handler (interrupt 4)
; System call number in R4 after interrupt
; Arguments in R5-R7
; Return value in R2

syscall_handler:
    ; R4 = 4 (interrupt number)
    ; Need to get actual syscall number from user code
    ; Option: use R5 for syscall number, R6-R7 for args

    ; For this example, syscall number in R5
    ADDI R26, R0, 1
    BEQ  R5, R26, sys_exit
    ADDI R26, R0, 2
    BEQ  R5, R26, sys_write
    ; ... more syscalls ...

    ; Unknown syscall - return error
    ADDI R2, R0, -1
    IRET

sys_exit:
    ; Exit program
    ; R6 = exit code
    ; Set running = 0 (halt VM)
    ; (Cannot directly set from assembly - would need VM support)
    IRET

sys_write:
    ; Write to console
    ; R6 = buffer address
    ; R7 = length
    ; ... implementation ...
    ADDI R2, R0, 0      ; Success
    IRET
```

#### Example 2: Timer Handler

```assembly
; Timer interrupt handler (interrupt 6)
timer_handler:
    ; Increment tick counter
    LUI  R26, 0
    ORI  R26, R26, 0x8000   ; Tick counter at 0x8000
    LW   R27, 0(R26)
    ADDI R27, R27, 1
    SW   R27, 0(R26)

    ; Check if timeslice expired (every 100 ticks)
    ANDI R27, R27, 0x63     ; Modulo 100
    BNE  R27, R0, timer_done

    ; Timeslice expired - trigger task switch
    RAISE 16                ; Task switch interrupt

timer_done:
    IRET
```

#### Example 3: Page Fault Handler

```assembly
; Page fault handler (interrupt 8)
page_fault_handler:
    ; Get faulting PC
    GETPC R26

    ; Decode instruction to get faulting address
    ; (Simplified - real handler would fully decode)
    LW   R27, 0(R26)        ; Load instruction

    ; Extract offset field (bytes 4-7)
    LW   R27, 4(R26)        ; Load immediate (offset)

    ; Get base register from instruction
    ; (Would need to extract rs field from byte 1)
    ; For this example, assume we know the virtual address

    ; Check if page should be loaded
    ; ... load page from disk ...
    ; ... update page table entry ...

    ; Mark page as valid
    ; ... set V bit in PTE ...

    ; Retry instruction
    IRET
```

### 8.9 Exception Handling

Exceptions are synchronous interrupts raised by the VM during instruction execution.

**Exception Types:**

1. **ILLEGAL_OP (1)** - Unknown opcode
```assembly
; Example: Invalid opcode 0xFF
.byte 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
; Raises interrupt 1
```

2. **MEM_FAULT (2)** - Out of bounds access
```assembly
; Load from address beyond memory size
LUI  R5, 0xFFFF
LW   R6, 0(R5)      ; Likely out of bounds
; Raises interrupt 2
```

3. **ALIGN_FAULT (3)** - Misaligned PC
```assembly
; Jump to misaligned address
ADDI R5, R0, 0x1001 ; Not 8-byte aligned
JR   R5             ; Raises interrupt 2 (MEM_FAULT)
```

4. **PRIVILEGE_VIOLATION (7)** - User mode executing privileged instruction
```assembly
; In user mode:
EI                  ; Privileged instruction
; Raises interrupt 7
```

5. **PAGE_FAULT (8)** - MMU translation failure
```assembly
; Access invalid page
LW  R5, 0x10000(R0)  ; Page not mapped
; Raises interrupt 8
```

**Exception Handler Pattern:**
```assembly
exception_handler:
    ; Check exception type (in R4)
    ADDI R26, R0, 1
    BEQ  R4, R26, handle_illegal_op
    ADDI R26, R0, 7
    BEQ  R4, R26, handle_privilege
    ADDI R26, R0, 8
    BEQ  R4, R26, handle_page_fault

    ; Default: terminate process
    J terminate_process

handle_illegal_op:
    ; Log error
    ; Kill process
    J terminate_process

handle_privilege:
    ; Security violation
    ; Log and kill process
    J terminate_process

handle_page_fault:
    ; Try to fix page table
    ; If cannot fix, kill process
    ; Otherwise IRET to retry

terminate_process:
    ; Cleanup process
    ; Schedule next process
    ; (VM-specific implementation)
    IRET
```

---

## 9. VM Implementation

This section describes the C89 virtual machine implementation details.

### 9.1 Implementation Philosophy

The CRISP-32 VM is implemented with the following principles:

**Freestanding C89:**
- No standard library dependencies for VM core
- Uses custom `c32_string.c` for memory operations
- Portable across any platform with a C89 compiler

**Source Files:**
- `include/c32_types.h` - Type definitions
- `include/c32_vm.h` - VM API
- `include/c32_opcodes.h` - Opcode definitions
- `src/vm/c32_vm.c` - VM implementation (freestanding)
- `src/vm/main.c` - Command-line runner (hosted)
- `src/common/c32_string.c` - Freestanding string functions

**Compilation:**
```bash
# VM core (freestanding)
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
    -ffreestanding -fno-builtin \
    -c -o c32_vm.o src/vm/c32_vm.c

# VM runner (hosted)
gcc -std=c89 -pedantic -Wall -Wextra -Werror \
    -o crisp32 src/vm/main.c c32_vm.o c32_string.o
```

### 9.2 VM Structure

**Core Structure Definition:**
```c
/* From include/c32_vm.h lines 43-85 */
typedef struct {
    uint32_t regs[32];         /* R0-R31 */
    uint32_t pc;               /* Program counter */
    uint8_t *memory;           /* Guest physical memory */
    uint32_t memory_size;      /* Memory size in bytes */
    int running;               /* 1 = running, 0 = halted */
    uint8_t kernel_mode;       /* 1 = kernel, 0 = user */
    uint8_t paging_enabled;    /* 1 = paging on */
    uint32_t page_table_base;  /* Physical address of page table */
    uint32_t num_pages;        /* Number of virtual pages */

    struct {
        uint8_t enabled;       /* Global interrupt enable */
        uint8_t pending[32];   /* 256-bit pending bitmap */
        uint32_t saved_pc;     /* Saved PC on interrupt */
        uint32_t saved_regs_addr; /* Stack address of saved regs */
    } interrupts;
} c32_vm_t;
```

**Size:** `sizeof(c32_vm_t)` varies by platform, typically ~200 bytes

**Memory Layout:**
- Registers: 32 × 4 = 128 bytes
- PC: 4 bytes
- Memory pointer: 4-8 bytes (platform-dependent)
- Flags and state: ~50-70 bytes

### 9.3 Initialization

**VM Initialization:**
```c
/* From src/vm/c32_vm.c lines 194-220 */
void c32_vm_init(c32_vm_t *vm, uint8_t *memory, uint32_t memory_size) {
    size_t i;

    /* Clear all registers */
    for (i = 0; i < 32; i++) {
        vm->regs[i] = 0;
    }

    vm->pc = 0;
    vm->memory = memory;
    vm->memory_size = memory_size;
    vm->running = 0;

    /* Start in kernel mode with paging disabled */
    vm->kernel_mode = 1;
    vm->paging_enabled = 0;
    vm->page_table_base = 0;
    vm->num_pages = 0;

    /* Clear interrupt state */
    vm->interrupts.enabled = 0;
    for (i = 0; i < 32; i++) {
        vm->interrupts.pending[i] = 0;
    }
    vm->interrupts.saved_pc = 0;
    vm->interrupts.saved_regs_addr = 0;
}
```

**Usage Example:**
```c
#include "c32_vm.h"

int main(void) {
    c32_vm_t vm;
    uint8_t memory[1024 * 1024];  /* 1 MB */

    /* Initialize VM */
    c32_vm_init(&vm, memory, sizeof(memory));

    /* Load program into memory */
    /* ... */

    /* Set PC to entry point */
    vm.pc = 0x1000;

    /* Run */
    c32_vm_run(&vm);

    return 0;
}
```

### 9.4 Instruction Execution Cycle

**Single Step:**
```c
/* Simplified from src/vm/c32_vm.c lines 526-948 */
int c32_vm_step(c32_vm_t *vm) {
    uint8_t opcode, rs, rt, rd;
    uint32_t imm;
    uint32_t phys_pc;
    const uint8_t *inst;

    /* 1. Check for pending interrupts */
    if (check_interrupts(vm) < 0) return -1;

    /* 2. Check PC alignment */
    if ((vm->pc & 0x7) != 0) {
        c32_raise_interrupt(vm, 2);
        return -1;
    }

    /* 3. Translate PC (virtual to physical) */
    phys_pc = translate_address(vm, vm->pc, 0, 1);
    if (phys_pc == 0xFFFFFFFF) return -1;

    /* 4. Check bounds */
    if (phys_pc + 8 > vm->memory_size) {
        vm->running = 0;
        return -1;
    }

    /* 5. Fetch instruction */
    inst = vm->memory + phys_pc;
    opcode = inst[0];
    rs = inst[1];
    rt = inst[2];
    rd = inst[3];
    imm = c32_read_word(inst + 4);

    /* 6. Advance PC */
    vm->pc += 8;

    /* 7. Execute */
    switch (opcode) {
        case OP_ADD:
            vm->regs[rd] = (int32_t)vm->regs[rs] + (int32_t)vm->regs[rt];
            break;
        /* ... all other opcodes ... */
    }

    /* 8. Enforce R0 = 0 */
    vm->regs[0] = 0;

    return 0;
}
```

**Run Loop:**
```c
/* From src/vm/c32_vm.c lines 962-970 */
void c32_vm_run(c32_vm_t *vm) {
    vm->running = 1;

    while (vm->running) {
        if (c32_vm_step(vm) != 0) {
            break;
        }
    }
}
```

### 9.5 Endianness Handling

CRISP-32 uses little-endian encoding. The VM automatically converts on big-endian hosts.

**Compile-Time Detection:**
```c
/* From src/vm/c32_vm.c lines 36-69 */
#ifdef C32_HOST_BIG_ENDIAN
#define NEED_ENDIAN_SWAP 1

static uint32_t swap32(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

static uint16_t swap16(uint16_t val) {
    return ((val & 0xFF00) >> 8) |
           ((val & 0x00FF) << 8);
}
#else
#define NEED_ENDIAN_SWAP 0
#endif
```

**Memory Access:**
```c
/* From src/vm/c32_vm.c lines 87-95 */
uint32_t c32_read_word(const uint8_t *addr) {
    uint32_t val;
    c32_memcpy(&val, addr, 4);
#if NEED_ENDIAN_SWAP
    return swap32(val);
#else
    return val;
#endif
}
```

**Big-Endian Compilation:**
```bash
gcc -DC32_HOST_BIG_ENDIAN -o crisp32 src/vm/*.c
```

### 9.6 Memory Management

**Host Memory Allocation:**
```c
/* Static allocation */
uint8_t memory[16 * 1024 * 1024];  /* 16 MB */
c32_vm_init(&vm, memory, sizeof(memory));

/* Dynamic allocation (requires stdlib - hosted only) */
uint8_t *memory = malloc(16 * 1024 * 1024);
c32_vm_init(&vm, memory, 16 * 1024 * 1024);
/* ... */
free(memory);
```

**Memory Layout Recommendations:**
```
0x00000000: IVT (2 KB)
0x00000800: Reserved (2 KB)
0x00001000: Kernel code
0x00005000: Kernel data
0x00009000: Page table
0x0000A000: User memory
```

**Bounds Checking:**
```c
/* Every memory access checks bounds */
if (phys_addr + size > vm->memory_size) {
    /* Out of bounds - raise exception */
    c32_raise_interrupt(vm, 2);
    return;
}
```

### 9.7 Performance Characteristics

**Instruction Throughput:**
- Simple instructions (ADD, OR): 1-2 host cycles per guest instruction
- Memory operations: 5-10 host cycles (with translation)
- Branch misprediction: ~10-20 host cycles
- Interrupt dispatch: ~100-200 host cycles

**Typical Performance:**
- Modern x86-64: 10-50 MIPS (million instructions per second)
- ARM Cortex-A: 5-20 MIPS
- Embedded systems: 1-5 MIPS

**Optimization Opportunities:**
1. **Instruction Caching**: Cache decoded instructions
2. **Threaded Interpretation**: Computed gotos instead of switch
3. **JIT Compilation**: Translate to host code
4. **Register Mapping**: Map guest registers to host registers

### 9.8 Portability

**Platform Requirements:**
- C89 compiler
- 32-bit or 64-bit CPU
- Any endianness (automatic conversion)
- ~200 bytes stack, ~16 MB+ heap (configurable)

**Tested Platforms:**
- Linux x86-64 (gcc, clang)
- macOS ARM64 (clang)
- Windows x64 (MSVC, MinGW)
- Embedded ARM (gcc)

**No Dependencies:**
- No libc (VM core)
- No OS services
- No threading
- No dynamic allocation in core

**Portability Considerations:**
```c
/* Do NOT assume sizeof(int) == 4 */
/* Use explicit uint32_t types */

/* Do NOT assume sizeof(void*) == 4 */
/* Store addresses as uint32_t in guest memory */

/* Do NOT assume little-endian */
/* Use c32_read_word/c32_write_word */

/* Do NOT use long long */
/* C89 doesn't guarantee it */
```

### 9.9 Testing

**Unit Tests:**

Located in `src/test/unit/`, automated test suite covers:
- All arithmetic instructions
- All logical operations
- Memory access (load/store)
- Branches and jumps
- Privilege separation
- MMU/paging

**Running Tests:**
```bash
make test

# Output:
# Running test suite (11 tests)...
# [1/11] ADD and ADDI instructions ... PASS
# [2/11] SUB instruction ... PASS
# ...
# All tests passed!
```

**Test Framework:**
```c
/* Example test: src/test/test_suite.c */
static int test_add_validation(c32_test_ctx_t *ctx) {
    /* R1 should be 42 */
    C32_ASSERT_REG_EQ(ctx, 1, 42);

    /* R2 should be 10 */
    C32_ASSERT_REG_EQ(ctx, 2, 10);

    /* R3 should be 52 (42 + 10) */
    C32_ASSERT_REG_EQ(ctx, 3, 52);

    /* VM should have halted */
    C32_ASSERT_HALTED(ctx);

    return C32_TEST_PASS;
}
```

**Test Documentation:** `src/test/README.md`

---

## 10. Assembly Language

CRISP-32 uses a simple, MIPS-like assembly syntax.

### 10.1 Syntax Overview

**Instruction Format:**
```
OPCODE destination, source1, source2/immediate
```

**Examples:**
```assembly
ADD  R5, R10, R12      ; R5 = R10 + R12
ADDI R3, R17, 42       ; R3 = R17 + 42
LW   R8, 100(R2)       ; R8 = mem[R2 + 100]
BEQ  R5, R6, label     ; if (R5 == R6) goto label
JAL  function          ; call function
```

**Register Notation:**
- Numeric: `R0` to `R31`
- Symbolic: `zero`, `sp`, `ra`, etc. (assembler-specific)

**Immediate Values:**
- Decimal: `42`, `-10`
- Hexadecimal: `0x2A`, `0xFFFFFFF6`
- Binary: `0b00101010` (assembler-specific)

**Labels:**
```assembly
start:              ; Label at current address
    ADDI R1, R0, 10
loop:
    ADDI R1, R1, -1
    BNE  R1, R0, loop
```

**Comments:**
```assembly
; Semicolon comment
# Hash comment (some assemblers)
ADD R5, R10, R12  ; End-of-line comment
```

### 10.2 Assembler Directives

While CRISP-32 doesn't mandate specific assembler directives, common conventions include:

**.org** - Set origin address
```assembly
.org 0x1000
start:
    ; Code starts at 0x1000
```

**.align** - Align to boundary
```assembly
.align 8            ; Align to 8-byte boundary
instruction:
    ADD R5, R10, R12
```

**.word** - Define 32-bit data
```assembly
.word 0x12345678    ; 4-byte constant
```

**.byte** - Define 8-bit data
```assembly
.byte 0x01, 0x02, 0x03, 0x04
```

**.space** - Reserve space
```assembly
.space 256          ; Reserve 256 bytes
```

**.equ** - Define constant
```assembly
.equ STACK_SIZE, 4096
.equ IVT_BASE, 0x0000
```

### 10.3 Pseudo-Instructions

Pseudo-instructions are assembler conveniences that expand to real instructions:

**NOP** - No operation
```assembly
NOP                 ; Expands to: ADD R0, R0, R0
```

**MOV** - Move register
```assembly
MOV  R5, R10        ; Expands to: OR R5, R10, R0
```

**LI** - Load immediate (32-bit)
```assembly
LI   R5, 0x12345678 ; If fits in immediate:
                    ;   ADDI R5, R0, 0x12345678
                    ; Otherwise might use LUI+ORI
```

**LA** - Load address
```assembly
LA   R5, label      ; Expands to: ADDI R5, R0, label_offset
```

**NOT** - Bitwise NOT
```assembly
NOT  R5, R10        ; Expands to: NOR R5, R10, R0
```

**NEG** - Negate
```assembly
NEG  R5, R10        ; Expands to: SUB R5, R0, R10
```

**B** - Unconditional branch
```assembly
B    label          ; Expands to: BEQ R0, R0, label
```

**PUSH** - Push register
```assembly
PUSH R5             ; Expands to:
                    ;   ADDI R29, R29, -4
                    ;   SW   R5, 0(R29)
```

**POP** - Pop register
```assembly
POP  R5             ; Expands to:
                    ;   LW   R5, 0(R29)
                    ;   ADDI R29, R29, 4
```

**CALL** - Function call
```assembly
CALL function       ; Expands to: JAL function
```

**RET** - Return from function
```assembly
RET                 ; Expands to: JR R31
```

### 10.4 Example Programs

#### Example 1: Hello World (Conceptual)

```assembly
; Hello World program
; Uses syscall convention: R5 = syscall number

.org 0x1000

start:
    ; Print string
    ADDI R5, R0, 4          ; Syscall 4: print string
    LUI  R6, 0
    ORI  R6, R6, msg        ; R6 = address of message
    SYSCALL

    ; Exit
    ADDI R5, R0, 10         ; Syscall 10: exit
    ADDI R6, R0, 0          ; Exit code 0
    SYSCALL

msg:
    .byte 'H', 'e', 'l', 'l', 'o', ',', ' '
    .byte 'W', 'o', 'r', 'l', 'd', '!', '\n', 0
```

#### Example 2: Fibonacci Sequence

```assembly
; Calculate Fibonacci sequence
; F(n) = F(n-1) + F(n-2)
; F(0) = 0, F(1) = 1

.org 0x1000

start:
    ADDI R10, R0, 10        ; Calculate F(10)
    JAL  fibonacci
    ; Result in R2
    SYSCALL                 ; Exit

fibonacci:
    ; Input: R10 = n
    ; Output: R2 = F(n)
    ; Uses: R11, R12, R13

    ; Base cases
    BEQ  R10, R0, fib_zero
    ADDI R11, R0, 1
    BEQ  R10, R11, fib_one

    ; Recursive case: F(n) = F(n-1) + F(n-2)
    ; Save registers
    ADDI R29, R29, -16
    SW   R10, 0(R29)        ; Save n
    SW   R31, 4(R29)        ; Save return address
    SW   R12, 8(R29)        ; Save temp
    SW   R13, 12(R29)       ; Save temp

    ; Calculate F(n-1)
    ADDI R10, R10, -1
    JAL  fibonacci
    OR   R12, R2, R0        ; R12 = F(n-1)

    ; Calculate F(n-2)
    LW   R10, 0(R29)        ; Restore n
    ADDI R10, R10, -2
    JAL  fibonacci
    OR   R13, R2, R0        ; R13 = F(n-2)

    ; Add results
    ADD  R2, R12, R13       ; R2 = F(n-1) + F(n-2)

    ; Restore and return
    LW   R10, 0(R29)
    LW   R31, 4(R29)
    LW   R12, 8(R29)
    LW   R13, 12(R29)
    ADDI R29, R29, 16
    JR   R31

fib_zero:
    ADDI R2, R0, 0
    JR   R31

fib_one:
    ADDI R2, R0, 1
    JR   R31
```

#### Example 3: Memory Copy

```assembly
; Memory copy function
; void memcpy(void *dest, void *src, size_t n)

.org 0x1000

start:
    ; Setup test
    LUI  R5, 0
    ORI  R5, R5, 0x2000     ; dest = 0x2000
    LUI  R6, 0
    ORI  R6, R6, 0x1000     ; src = 0x1000
    ADDI R7, R0, 256        ; n = 256 bytes
    JAL  memcpy
    SYSCALL                 ; Exit

memcpy:
    ; R5 = dest
    ; R6 = src
    ; R7 = n (bytes)

    ; Check if n == 0
    BEQ  R7, R0, memcpy_done

    ; Calculate end address
    ADD  R8, R6, R7         ; R8 = src + n

memcpy_loop:
    ; Copy byte
    LBU  R9, 0(R6)          ; Load byte from src
    SB   R9, 0(R5)          ; Store byte to dest

    ; Increment pointers
    ADDI R5, R5, 1
    ADDI R6, R6, 1

    ; Check if done
    BNE  R6, R8, memcpy_loop

memcpy_done:
    JR   R31
```

### 10.5 Calling Convention

While not enforced by hardware, CRISP-32 follows a standard calling convention:

**Register Usage:**
- `R0` (zero): Always zero
- `R1` (at): Assembler temporary (not preserved)
- `R2-R3` (v0-v1): Return values
- `R4-R7` (a0-a3): Function arguments
- `R8-R15` (t0-t7): Temporaries (caller-saved)
- `R16-R23` (s0-s7): Saved registers (callee-saved)
- `R24-R25` (t8-t9): More temporaries (caller-saved)
- `R26-R27` (k0-k1): Kernel reserved
- `R28` (gp): Global pointer (if used)
- `R29` (sp): Stack pointer
- `R30` (fp): Frame pointer (if used)
- `R31` (ra): Return address

**Function Prologue:**
```assembly
function:
    ; Save return address and callee-saved registers
    ADDI R29, R29, -32      ; Allocate stack frame
    SW   R31, 28(R29)       ; Save return address
    SW   R16, 24(R29)       ; Save s0
    SW   R17, 20(R29)       ; Save s1
    ; ... save other registers used ...

    ; Function body
    ; ...
```

**Function Epilogue:**
```assembly
    ; Restore registers
    LW   R16, 24(R29)
    LW   R17, 20(R29)
    LW   R31, 28(R29)
    ADDI R29, R29, 32       ; Deallocate stack frame
    JR   R31                ; Return
```

**Calling a Function:**
```assembly
    ; Setup arguments
    ADDI R4, R0, 10         ; a0 = 10
    ADDI R5, R0, 20         ; a1 = 20

    ; Save caller-saved registers if needed
    ADDI R29, R29, -8
    SW   R8, 0(R29)
    SW   R9, 4(R29)

    ; Call function
    JAL  function

    ; Result in R2 (v0)
    OR   R10, R2, R0        ; Save result

    ; Restore caller-saved registers
    LW   R8, 0(R29)
    LW   R9, 4(R29)
    ADDI R29, R29, 8
```

### 10.6 Inline Assembly (Conceptual)

For systems with C compilers targeting CRISP-32:

```c
/* Inline assembly syntax (compiler-specific) */

/* Read register */
uint32_t get_sp(void) {
    uint32_t sp;
    __asm__("OR %0, R29, R0" : "=r"(sp));
    return sp;
}

/* Write register */
void set_sp(uint32_t sp) {
    __asm__("OR R29, %0, R0" : : "r"(sp));
}

/* Atomic operations */
void disable_interrupts(void) {
    __asm__("DI");
}

void enable_interrupts(void) {
    __asm__("EI");
}
```

---

## 11. Programming Guide

This section provides practical guidance for programming CRISP-32.

### 11.1 Getting Started

**Minimal Program:**
```assembly
; Minimal CRISP-32 program
.org 0x1000

start:
    ; Your code here
    ADDI R1, R0, 42         ; R1 = 42
    SYSCALL                 ; Exit
```

**Compiling:**
```bash
bin/c32asm program.asm program.bin
```

**Running:**
```bash
bin/crisp32 program.bin
```

**Output:**
```
Loaded 8 bytes from 'program.bin' at address 0x00001000

Starting execution at 0x00001000...

Program halted after 2 steps

Register State:
R0 : 0x00000000  R1 : 0x0000002A  R2 : 0x00000000  R3 : 0x00000000
...
```

### 11.2 Common Patterns

#### Pattern 1: Loop

```assembly
; Count from 0 to 9
    ADDI R1, R0, 0          ; Counter

loop:
    ; Loop body
    ADDI R1, R1, 1          ; Increment

    ; Check condition
    ADDI R2, R0, 10
    BNE  R1, R2, loop       ; Continue if R1 != 10

    ; After loop
```

#### Pattern 2: Conditional

```assembly
; if (R1 < R2) { R3 = 1; } else { R3 = 0; }
    SLT  R4, R1, R2         ; R4 = (R1 < R2)
    BEQ  R4, R0, else_branch

then_branch:
    ADDI R3, R0, 1
    J    endif

else_branch:
    ADDI R3, R0, 0

endif:
```

#### Pattern 3: Switch Statement

```assembly
; switch (R1) { case 0: ... case 1: ... default: ... }
    BEQ  R1, R0, case_0
    ADDI R2, R0, 1
    BEQ  R1, R2, case_1
    ADDI R2, R0, 2
    BEQ  R1, R2, case_2
    J    default_case

case_0:
    ; Handle case 0
    J    end_switch

case_1:
    ; Handle case 1
    J    end_switch

case_2:
    ; Handle case 2
    J    end_switch

default_case:
    ; Handle default

end_switch:
```

#### Pattern 4: Array Access

```assembly
; array[index] access
; R10 = base address, R11 = index

    SLL  R12, R11, 2        ; R12 = index * 4 (word size)
    ADD  R12, R10, R12      ; R12 = base + offset
    LW   R5, 0(R12)         ; R5 = array[index]
```

#### Pattern 5: Struct Member Access

```assembly
; struct { int a; int b; int c; } *ptr;
; ptr->b access

    ; ptr in R10
    LW   R5, 4(R10)         ; R5 = ptr->b (offset 4)
```

### 11.3 Optimization Techniques

#### 1. Register Allocation

**Bad:**
```assembly
; Excessive memory access
LW   R1, value
ADDI R1, R1, 1
SW   R1, value
LW   R1, value
ADDI R1, R1, 1
SW   R1, value
```

**Good:**
```assembly
; Keep in register
LW   R1, value
ADDI R1, R1, 1
ADDI R1, R1, 1
SW   R1, value
```

#### 2. Loop Unrolling

**Bad:**
```assembly
; Process array one element at a time
loop:
    LW   R5, 0(R10)
    ADD  R6, R6, R5
    ADDI R10, R10, 4
    ADDI R11, R11, -1
    BNE  R11, R0, loop
```

**Good:**
```assembly
; Process 4 elements per iteration
loop:
    LW   R5, 0(R10)
    LW   R7, 4(R10)
    LW   R8, 8(R10)
    LW   R9, 12(R10)
    ADD  R6, R6, R5
    ADD  R6, R6, R7
    ADD  R6, R6, R8
    ADD  R6, R6, R9
    ADDI R10, R10, 16
    ADDI R11, R11, -4
    BGTZ R11, loop
```

#### 3. Strength Reduction

**Bad:**
```assembly
; Multiply by 8
MUL  R5, R1, R2         ; where R2 = 8
```

**Good:**
```assembly
; Shift instead
SLL  R5, R1, 3          ; R5 = R1 << 3 (multiply by 8)
```

#### 4. Constant Propagation

**Bad:**
```assembly
ADDI R1, R0, 10
ADDI R2, R0, 5
ADD  R3, R1, R2
```

**Good:**
```assembly
ADDI R3, R0, 15         ; 10 + 5 computed at compile time
```

### 11.4 Common Pitfalls

#### Pitfall 1: Forgetting R0 is Zero

**Wrong:**
```assembly
ADDI R0, R0, 1          ; R0 is always 0!
LW   R5, 0(R0)          ; OK - R0 as base address
```

**Right:**
```assembly
ADDI R1, R0, 1          ; Use R1
LW   R5, 0(R1)          ; Load from address in R1
```

#### Pitfall 2: PC Misalignment

**Wrong:**
```assembly
.org 0x1001             ; Not 8-byte aligned!
start:
    ADD R5, R10, R12
```

**Right:**
```assembly
.org 0x1000             ; 8-byte aligned
start:
    ADD R5, R10, R12
```

#### Pitfall 3: Branch Offset Calculation

**Wrong:**
```assembly
; Assuming offset is in instructions
BEQ  R5, R6, 2          ; Offset 2 (wrong!)
```

**Right:**
```assembly
; Offset is in bytes
BEQ  R5, R6, 16         ; Skip 2 instructions (2 × 8 = 16 bytes)
; Or use label:
BEQ  R5, R6, target
target:
```

#### Pitfall 4: Stack Corruption

**Wrong:**
```assembly
function:
    SW   R5, 0(R29)     ; Overwrites caller's stack!
    ; ...
```

**Right:**
```assembly
function:
    ADDI R29, R29, -4   ; Allocate space
    SW   R5, 0(R29)     ; Now safe
    ; ...
    LW   R5, 0(R29)
    ADDI R29, R29, 4    ; Deallocate
```

#### Pitfall 5: Not Saving Callee-Saved Registers

**Wrong:**
```assembly
function:
    OR   R16, R5, R0    ; Use R16 without saving
    ; ...
    JR   R31
```

**Right:**
```assembly
function:
    ADDI R29, R29, -4
    SW   R16, 0(R29)    ; Save R16
    OR   R16, R5, R0
    ; ...
    LW   R16, 0(R29)    ; Restore R16
    ADDI R29, R29, 4
    JR   R31
```

### 11.5 Debugging

**Debugging Techniques:**

1. **Breakpoint Instruction**
```assembly
    BREAK               ; Raises interrupt 5
```

2. **Trace Points**
```assembly
    ; Write trace info to memory
    ADDI R26, R0, 0x1234    ; Trace value
    LUI  R27, 0
    ORI  R27, R27, 0xF000   ; Trace buffer
    SW   R26, 0(R27)        ; Store trace
```

3. **Register Dumps**
```assembly
; Save all registers for inspection
save_regs:
    LUI  R26, 0
    ORI  R26, R26, 0xE000   ; Dump area
    SW   R0, 0(R26)
    SW   R1, 4(R26)
    ; ... all registers ...
    SW   R31, 124(R26)
```

4. **Assertions**
```assembly
; Assert R5 == 42
    ADDI R26, R0, 42
    BEQ  R5, R26, assert_ok
    BREAK               ; Assertion failed
assert_ok:
```

### 11.6 Performance Tips

1. **Minimize Memory Access**
   - Keep frequently-used values in registers
   - Use register-to-register operations when possible

2. **Avoid Pipeline Stalls** (future hardware)
   - Don't use result of load immediately
   - Fill branch delay slots (if implemented)

3. **Align Data**
   - Word-align word data
   - Halfword-align halfword data

4. **Use Appropriate Data Types**
   - Use byte loads (LB/LBU) for byte data
   - Don't load words if only need bytes

5. **Minimize Branches**
   - Use conditional moves if available
   - Unroll small loops

6. **Function Inlining**
   - Inline small functions
   - Reduces call overhead

---

## 12. Design Rationale

This section explains the reasoning behind key CRISP-32 design decisions.

### 12.1 Why 64-bit Instructions?

**Problem:** How to encode full 32-bit immediates in a RISC architecture?

**Traditional RISC Approach (MIPS, ARM):**
- 32-bit instructions
- Limited immediate size (16 bits)
- Requires LUI + ORI for full 32-bit constants
- More code, more instructions

**CRISP-32 Approach:**
- 64-bit instructions
- Full 32-bit immediates
- Single instruction for any constant

**Trade-offs:**
| Aspect | 32-bit | 64-bit (CRISP-32) |
|--------|--------|-------------------|
| Code size | Smaller | 2x larger |
| Constant loading | 2 instructions | 1 instruction |
| Decode complexity | Moderate | Simple |
| Opcode space | Limited (6-8 bits) | Large (8 bits = 256) |
| Cache efficiency | Better | Worse |
| VM simplicity | Moderate | Excellent |

**Conclusion:** For a VM-based ISA, decode simplicity and programming convenience outweigh code size concerns.

### 12.2 Why Little-Endian?

**Rationale:**
1. **Industry Standard**: x86, ARM (default), RISC-V are little-endian
2. **Simplicity**: Byte 0 at address 0
3. **Consistency**: Natural byte ordering

**Automatic Conversion:**
- Big-endian hosts: `#define C32_HOST_BIG_ENDIAN`
- VM automatically byte-swaps
- Zero performance impact on little-endian hosts (99% of systems)

### 12.3 Why R0 Hardwired to Zero?

**Benefits:**
1. **Load Constants**: `ADDI R5, R0, 42` loads 42
2. **Clear Registers**: `OR R5, R0, R0` clears R5
3. **Pseudo-Instructions**: Enables MOV, NOT, NEG, etc.
4. **Unconditional Branch**: `BEQ R0, R0, label`
5. **Common Pattern**: Used in MIPS, RISC-V

**Cost:** One less general-purpose register (still have 31)

### 12.4 Why Byte Offsets for Branches?

**Rationale:**
1. **Consistency**: Memory operations use byte offsets
2. **Flexibility**: Can branch to data or unaligned addresses (if needed)
3. **Full Range**: Can branch ±2GB with 32-bit offset

**Alternative (Instruction Offsets):**
- MIPS uses instruction offsets
- Saves one bit (shift offset left by 2)
- Less intuitive for programmers
- CRISP-32 has full 32-bit offset, so no need to save bits

### 12.5 Why Two Privilege Levels?

**Rationale:**
1. **Sufficient**: Kernel vs user is enough for most systems
2. **Simple**: Easy to understand and implement
3. **Performance**: Minimal overhead

**Alternative (More Levels):**
- x86 has 4 levels (rings 0-3)
- Most OSes only use 2 (ring 0 and ring 3)
- Added complexity for little benefit

### 12.6 Why Simple Page Table?

**Rationale:**
1. **Simplicity**: Single-level, flat table
2. **VM Efficiency**: Fast translation in software
3. **Predictability**: Constant-time lookup

**Alternative (Multi-Level):**
- x86, ARM use multi-level page tables
- Saves memory for sparse address spaces
- More complex, slower translation
- CRISP-32 is VM-based: memory is cheap, speed matters

### 12.7 Why Freestanding C89?

**Rationale:**
1. **Portability**: C89 available on all platforms
2. **Simplicity**: No runtime dependencies
3. **Embedded**: Works on bare metal
4. **Deterministic**: No hidden allocations or overhead

**Cost:**
- No 64-bit types (affects MULH/MULHU)
- Manual string functions
- More verbose code

**Conclusion:** Portability and simplicity worth the trade-off.

### 12.8 Why Save All Registers on Interrupt?

**Rationale:**
1. **Simplicity**: Handler doesn't need to know what to save
2. **Correctness**: Guaranteed full context save
3. **VM Efficiency**: Batch save faster than selective

**Alternative (Selective Save):**
- x86 only saves minimal state
- Handler saves additional registers
- Faster for simple handlers
- More complex, error-prone

**Trade-off:**
- CRISP-32: 128 bytes saved (32 regs × 4 bytes)
- Simple, predictable, correct
- Cost: ~32 memory writes per interrupt

---

## 13. Appendices

### Appendix A: Opcode Reference Table

**Quick Opcode Lookup:**

| Opcode | Mnemonic | Type | Description |
|--------|----------|------|-------------|
| 0x00 | NOP | - | No operation |
| 0x01 | ADD | R | Add signed |
| 0x02 | ADDU | R | Add unsigned |
| 0x03 | SUB | R | Subtract signed |
| 0x04 | SUBU | R | Subtract unsigned |
| 0x05 | ADDI | I | Add immediate signed |
| 0x06 | ADDIU | I | Add immediate unsigned |
| 0x10 | AND | R | Bitwise AND |
| 0x11 | OR | R | Bitwise OR |
| 0x12 | XOR | R | Bitwise XOR |
| 0x13 | NOR | R | Bitwise NOR |
| 0x14 | ANDI | I | AND immediate |
| 0x15 | ORI | I | OR immediate |
| 0x16 | XORI | I | XOR immediate |
| 0x17 | LUI | I | Load upper immediate |
| 0x20 | SLL | R | Shift left logical |
| 0x21 | SRL | R | Shift right logical |
| 0x22 | SRA | R | Shift right arithmetic |
| 0x23 | SLLV | R | Shift left logical variable |
| 0x24 | SRLV | R | Shift right logical variable |
| 0x25 | SRAV | R | Shift right arithmetic variable |
| 0x30 | SLT | R | Set on less than signed |
| 0x31 | SLTU | R | Set on less than unsigned |
| 0x32 | SLTI | I | Set on less than immediate signed |
| 0x33 | SLTIU | I | Set on less than immediate unsigned |
| 0x40 | MUL | R | Multiply (lower 32 bits) |
| 0x41 | MULH | R | Multiply high signed |
| 0x42 | MULHU | R | Multiply high unsigned |
| 0x43 | DIV | R | Divide signed |
| 0x44 | DIVU | R | Divide unsigned |
| 0x45 | REM | R | Remainder signed |
| 0x46 | REMU | R | Remainder unsigned |
| 0x50 | LW | M | Load word |
| 0x51 | LH | M | Load halfword signed |
| 0x52 | LHU | M | Load halfword unsigned |
| 0x53 | LB | M | Load byte signed |
| 0x54 | LBU | M | Load byte unsigned |
| 0x58 | SW | M | Store word |
| 0x59 | SH | M | Store halfword |
| 0x5A | SB | M | Store byte |
| 0x60 | BEQ | B | Branch if equal |
| 0x61 | BNE | B | Branch if not equal |
| 0x62 | BLEZ | B | Branch if ≤ zero |
| 0x63 | BGTZ | B | Branch if > zero |
| 0x64 | BLTZ | B | Branch if < zero |
| 0x65 | BGEZ | B | Branch if ≥ zero |
| 0x70 | J | J | Jump |
| 0x71 | JAL | J | Jump and link |
| 0x72 | JR | J | Jump register |
| 0x73 | JALR | J | Jump and link register |
| 0xF0 | SYSCALL | S | System call |
| 0xF1 | BREAK | S | Breakpoint |
| 0xF2 | EI | P | Enable interrupts |
| 0xF3 | DI | P | Disable interrupts |
| 0xF4 | IRET | P | Interrupt return |
| 0xF5 | RAISE | S | Raise interrupt |
| 0xF6 | GETPC | S | Get saved PC |
| 0xF7 | ENABLE_PAGING | P | Enable paging |
| 0xF8 | DISABLE_PAGING | P | Disable paging |
| 0xF9 | SET_PTBR | P | Set page table base |
| 0xFB | ENTER_USER | P | Enter user mode |
| 0xFC | GETMODE | S | Get current mode |

**Type Legend:**
- R: Register-Register
- I: Immediate
- M: Memory
- B: Branch
- J: Jump
- S: System
- P: Privileged

### Appendix B: Register Quick Reference

| Number | Name | Alias | Usage | Saved? |
|--------|------|-------|-------|--------|
| R0 | zero | - | Constant 0 | N/A |
| R1 | at | - | Assembler temp | No |
| R2 | v0 | - | Return value 0 | No |
| R3 | v1 | - | Return value 1 | No |
| R4 | a0 | - | Argument 0 | No |
| R5 | a1 | - | Argument 1 | No |
| R6 | a2 | - | Argument 2 | No |
| R7 | a3 | - | Argument 3 | No |
| R8 | t0 | - | Temporary 0 | No |
| R9 | t1 | - | Temporary 1 | No |
| R10 | t2 | - | Temporary 2 | No |
| R11 | t3 | - | Temporary 3 | No |
| R12 | t4 | - | Temporary 4 | No |
| R13 | t5 | - | Temporary 5 | No |
| R14 | t6 | - | Temporary 6 | No |
| R15 | t7 | - | Temporary 7 | No |
| R16 | s0 | - | Saved 0 | Yes |
| R17 | s1 | - | Saved 1 | Yes |
| R18 | s2 | - | Saved 2 | Yes |
| R19 | s3 | - | Saved 3 | Yes |
| R20 | s4 | - | Saved 4 | Yes |
| R21 | s5 | - | Saved 5 | Yes |
| R22 | s6 | - | Saved 6 | Yes |
| R23 | s7 | - | Saved 7 | Yes |
| R24 | t8 | - | Temporary 8 | No |
| R25 | t9 | - | Temporary 9 | No |
| R26 | k0 | - | Kernel reserved 0 | No |
| R27 | k1 | - | Kernel reserved 1 | No |
| R28 | gp | - | Global pointer | Yes |
| R29 | sp | - | Stack pointer | Yes |
| R30 | fp | - | Frame pointer | Yes |
| R31 | ra | - | Return address | No |

### Appendix C: Exception/Interrupt Numbers

| Number | Name | Description | Source |
|--------|------|-------------|--------|
| 0 | RESET | System reset | External |
| 1 | ILLEGAL_OP | Illegal opcode | VM |
| 2 | MEM_FAULT | Memory fault | VM |
| 3 | ALIGN_FAULT | Alignment fault | VM |
| 4 | SYSCALL | System call | SYSCALL instruction |
| 5 | BREAK | Breakpoint | BREAK instruction |
| 6 | TIMER | Timer | External |
| 7 | PRIVILEGE_VIOLATION | Privilege violation | VM |
| 8 | PAGE_FAULT | Page fault | MMU |
| 9-15 | - | Reserved | - |
| 16-127 | - | Software interrupts | RAISE instruction |
| 128-255 | - | Hardware interrupts | External devices |

### Appendix D: Memory Map Example

**Typical 16MB System:**

```
Address Range          Size    Description
────────────────────────────────────────────────────
0x00000000-0x000007FF  2 KB    Interrupt Vector Table
0x00000800-0x00000FFF  2 KB    Reserved
0x00001000-0x00004FFF  16 KB   Kernel code
0x00005000-0x00007FFF  12 KB   Kernel data
0x00008000-0x00008FFF  4 KB    Kernel stack
0x00009000-0x00009FFF  4 KB    Page table (1024 entries)
0x0000A000-0x003FFFFF  ~4 MB   User pages (mapped)
0x00400000-0x00FFFFFF  ~12 MB  Unmapped / free
```

### Appendix E: Page Table Entry Format

**32-bit PTE Layout:**

```
Bit 31-12: Physical Page Number (PPN)
Bit 11-4:  Reserved (must be 0)
Bit 3:     U (User accessible)
Bit 2:     X (Executable)
Bit 1:     W (Writable)
Bit 0:     V (Valid)
```

**Common PTE Values:**

| Value | Binary | Permissions | Use Case |
|-------|--------|-------------|----------|
| 0x00000000 | ...0000 | Invalid | Unmapped page |
| 0x0000000F | ...1111 | U X W V | User code/data |
| 0x0000000D | ...1101 | U X - V | User code (R-X) |
| 0x0000000B | ...1011 | U - W V | User data (RW-) |
| 0x00000007 | ...0111 | - X W V | Kernel only |

### Appendix F: Instruction Encoding Examples

**Format Diagram:**
```
Bytes:  0       1       2       3       4       5       6       7
       ┌───────┬───────┬───────┬───────┬───────────────────────────┐
       │OP     │ RS    │ RT    │ RD    │ IMMEDIATE                 │
       └───────┴───────┴───────┴───────┴───────────────────────────┘
Bits:   7     0 7     0 7     0 7     0 7                         0
```

**Example: ADD R5, R10, R12**
```
Bytes:  01      0A      0C      05      00 00 00 00
Binary: 00000001 00001010 00001100 00000101 [32 zero bits]
```

**Example: ADDI R3, R17, 42**
```
Bytes:  05      11      03      00      2A 00 00 00
Binary: 00000101 00010001 00000011 00000000 00101010 [24 zero bits]
```

**Example: LW R8, 100(R2)**
```
Bytes:  50      02      08      00      64 00 00 00
Binary: 01010000 00000010 00001000 00000000 01100100 [24 zero bits]
```

### Appendix G: Performance Benchmarks

**VM Performance (Typical x86-64 Host):**

| Operation | Host Cycles | MIPS Equivalent |
|-----------|-------------|-----------------|
| ADD/OR/XOR | 1-2 | ~1 GHz host = 500-1000 MIPS |
| ADDI/ORI | 1-2 | ~1 GHz host = 500-1000 MIPS |
| LW/SW (no paging) | 3-5 | ~200-300 MIPS |
| LW/SW (with paging) | 8-12 | ~80-120 MIPS |
| BEQ (taken) | 10-20 | ~50-100 MIPS |
| BEQ (not taken) | 2-3 | ~300-500 MIPS |
| JAL/JR | 2-4 | ~250-500 MIPS |
| SYSCALL | 100-200 | ~5-10 MIPS |

**Notes:**
- Measurements vary by host CPU
- Modern x86-64: ~10-50 MIPS typical
- ARM Cortex-A: ~5-20 MIPS typical
- Embedded systems: ~1-5 MIPS typical

### Appendix H: C89 vs C99/C11

**CRISP-32 VM uses strict C89 for maximum portability.**

**Key Differences:**

| Feature | C89 | C99/C11 | CRISP-32 |
|---------|-----|---------|----------|
| `long long` | No | Yes | No - uses 32-bit workaround |
| `stdint.h` | No | Yes | Manual typedefs |
| `//` comments | No | Yes | Only `/* */` |
| Variable declarations | Start of block | Anywhere | Start of block |
| `inline` keyword | No | Yes | Manual inlining |
| VLA | No | Yes (C99) | Fixed arrays |

**MULH/MULHU Implementation:**

C99 version (ideal):
```c
uint32_t mulh(uint32_t a, uint32_t b) {
    uint64_t product = (uint64_t)a * (uint64_t)b;
    return (uint32_t)(product >> 32);
}
```

C89 version (actual):
```c
/* Karatsuba-like algorithm using 32-bit arithmetic */
/* See src/vm/c32_vm.c lines 667-704 */
```

### Appendix I: Building CRISP-32

**Build System:**

```bash
# Build everything
make

# Build specific targets
make vm          # VM only
make asm         # Assembler only
make tools       # Development tools
make test        # Build and run tests

# Debug build
make debug

# Clean
make clean
```

**Directory Structure:**
```
CRISP-32/
├── Makefile              # Build system
├── README.md             # Project overview
├── doc/
│   └── crisp32-spec.md   # This document
├── include/              # Public headers
│   ├── c32_types.h
│   ├── c32_vm.h
│   ├── c32_opcodes.h
│   ├── c32_asm.h
│   ├── c32_string.h
│   └── c32_test.h
├── src/
│   ├── vm/
│   │   ├── main.c
│   │   └── c32_vm.c
│   ├── asm/
│   │   ├── c32asm.c
│   │   ├── c32_parser.c
│   │   ├── c32_symbols.c
│   │   └── c32_encode.c
│   ├── test/
│   │   ├── README.md
│   │   ├── test_suite.c
│   │   ├── test_runner.c
│   │   └── unit/         # Unit tests
│   ├── tools/
│   │   └── bin2h.c
│   └── common/
│       └── c32_string.c
├── build/                # Build artifacts (generated)
└── bin/                  # Executables (generated)
    ├── crisp32           # VM
    ├── c32asm            # Assembler
    ├── bin2h             # Binary to header tool
    └── test_suite        # Test runner
```

**Dependencies:**
- C89 compiler (gcc, clang, MSVC)
- Make (GNU make recommended)
- POSIX environment (for build tools)

**No Runtime Dependencies:**
- VM core is freestanding
- No libc required for VM
- No external libraries

### Appendix J: Further Resources

**Source Code:**
- GitHub: [CRISP-32 Repository]
- License: GNU GPL v3

**Documentation:**
- ISA Specification: `doc/crisp32-spec.md` (this document)
- Test Framework: `src/test/README.md`
- VSCode Setup: `.vscode/README.md`

**Related Projects:**
- RISC-V: Modern open RISC ISA
- MIPS: Classic RISC architecture
- ARM: Industry-standard embedded ISA

**Contact:**
- Author: Manny Peterson
- Email: manny@manny.ca
- Year: 2025

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2025-01 | Initial release |
| 2.0 | 2025-01 | Complete rewrite - comprehensive documentation |

---

**End of CRISP-32 ISA Specification**

*This document comprehensively describes the CRISP-32 instruction set architecture, virtual machine implementation, and programming model. For implementation details, see the source code in `src/vm/c32_vm.c`. For usage examples, see the unit tests in `src/test/unit/`.*
