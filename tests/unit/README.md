# CRISP-32 Unit Testing Framework

A JUnit-style unit testing framework for CRISP-32 that validates VM instruction execution through assembly test programs.

## Overview

The testing framework follows a test-driven development approach:
1. Write assembly test programs (`.asm` files)
2. Define expected outcomes in C validation functions
3. Framework automatically assembles, embeds, and executes tests
4. Assertions validate register values, memory state, and VM status

## Quick Start

### Run All Tests
```bash
make test
```

### Build Tests Only
```bash
make test_build
```

### Clean Test Artifacts
```bash
make clean
```

## Test Structure

### Test Program Files

Each test consists of an assembly program (`.asm`):

```assembly
# tests/unit/test_add.asm
# Expected: R1=42, R2=10, R3=52

start:
    ADDI R1, R0, 42     # R1 = 42
    ADDI R2, R0, 10     # R2 = 10
    ADD R3, R1, R2      # R3 = R1 + R2 = 52
    SYSCALL             # Halt
```

### Test Validation Functions

Each test program has a corresponding validation function in `src/test/test_suite.c`:

```c
static int test_add_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 42);
    C32_ASSERT_REG_EQ(ctx, 2, 10);
    C32_ASSERT_REG_EQ(ctx, 3, 52);
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}
```

### Test Case Definition

Tests are registered in the test suite array:

```c
static const c32_test_case_t test_suite[] = {
    {
        "ADD and ADDI instructions",    // Test name
        test_add,                        // Binary data (from .h file)
        test_add_size,                   // Size of binary
        0x1000,                          // Load address
        100,                             // Max execution steps
        test_add_validation              // Validation function
    },
    // ... more tests
};
```

## Assertion API

The framework provides JUnit-style assertion macros:

### Register Assertions

```c
C32_ASSERT_REG_EQ(ctx, reg, expected)
```
Asserts that register `reg` equals `expected` value.

**Example:**
```c
C32_ASSERT_REG_EQ(ctx, 1, 42);  // Assert R1 == 42
```

### Memory Assertions

```c
C32_ASSERT_MEM_BYTE_EQ(ctx, addr, expected)
C32_ASSERT_MEM_WORD_EQ(ctx, addr, expected)
```
Asserts that memory at `addr` equals `expected` value.

**Example:**
```c
C32_ASSERT_MEM_WORD_EQ(ctx, 0x2000, 0x12345678);
```

### Program Counter Assertions

```c
C32_ASSERT_PC_EQ(ctx, expected)
```
Asserts that the program counter equals `expected` value.

### VM State Assertions

```c
C32_ASSERT_HALTED(ctx)
C32_ASSERT_RUNNING(ctx)
```
Asserts that the VM is in the expected running state.

### Custom Failures

```c
C32_ASSERT_FAIL(ctx, "Custom error message")
```
Immediately fails the test with a custom message.

## Example Tests

### Test: Arithmetic Operations

**File:** `test_add.asm`
```assembly
start:
    ADDI R1, R0, 42
    ADDI R2, R0, 10
    ADD R3, R1, R2
    SYSCALL
```

**Validation:**
```c
C32_ASSERT_REG_EQ(ctx, 1, 42);
C32_ASSERT_REG_EQ(ctx, 2, 10);
C32_ASSERT_REG_EQ(ctx, 3, 52);
```

### Test: Branch Instructions

**File:** `test_branch.asm`
```assembly
start:
    ADDI R1, R0, 5
    ADDI R2, R0, 5
    ADDI R3, R0, 0
    BEQ R1, R2, taken
    ADDI R3, R0, 99        # Should not execute
    SYSCALL

taken:
    ADDI R3, R0, 1         # Indicates branch was taken
    SYSCALL
```

**Validation:**
```c
C32_ASSERT_REG_EQ(ctx, 3, 1);  // Verify branch was taken
```

### Test: Memory Operations

**File:** `test_load_store.asm`
```assembly
start:
    LUI R1, 0x1234
    ORI R1, R1, 0x5678
    SW R1, R0, 0x2000      # Store to memory
    LW R2, R0, 0x2000      # Load from memory
    SYSCALL
```

**Validation:**
```c
C32_ASSERT_REG_EQ(ctx, 1, 0x12345678);
C32_ASSERT_REG_EQ(ctx, 2, 0x12345678);
C32_ASSERT_MEM_WORD_EQ(ctx, 0x2000, 0x12345678);
```

## Build Pipeline

The test framework uses an automated build pipeline:

1. **Assembly** (`.asm` → `.bin`)
   - Assembler converts test programs to binary

2. **Embedding** (`.bin` → `.h`)
   - `bin2h` tool converts binaries to C arrays
   - Generated headers define `test_<name>[]` arrays

3. **Compilation**
   - Test suite includes generated headers
   - Test runner compiles with VM core

4. **Execution**
   - Test suite runs each test program
   - Validates results with assertion macros
   - Reports pass/fail statistics

## Adding New Tests

### Step 1: Write Assembly Test

Create `tests/unit/test_name.asm`:
```assembly
# Describe what the test validates
# Expected results: R1=..., R2=...

start:
    # Your test code
    SYSCALL
```

### Step 2: Add Validation Function

In `src/test/test_suite.c`:
```c
#include "../../tests/unit/test_name.h"

static int test_name_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, expected_value);
    // ... more assertions
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}
```

### Step 3: Register Test Case

Add to `test_suite[]` array in `src/test/test_suite.c`:
```c
{
    "Test description",
    test_name,
    test_name_size,
    0x1000,
    100,
    test_name_validation
}
```

### Step 4: Build and Run

```bash
make test
```

The build system automatically:
- Assembles your `.asm` file
- Generates the `.h` header
- Compiles and links the test suite
- Runs all tests and reports results

## Test Output

### Success Example
```
Running test suite (7 tests)...

[1/7] ADD and ADDI instructions ... PASS
[2/7] SUB instruction ... PASS
[3/7] MUL instruction ... PASS
[4/7] Logical operations (AND, OR, XOR) ... PASS
[5/7] Shift operations (SLL, SRL) ... PASS
[6/7] Branch instructions (BEQ) ... PASS
[7/7] Load/Store instructions (LW, SW) ... PASS

========================================
Test Results Summary
========================================
Total:  7
Passed: 7
Failed: 0
Errors: 0
========================================
All tests passed!
```

### Failure Example
```
[3/7] MUL instruction ... FAIL
    Register R3: expected 0x0000002a, got 0x00000000
```

## Test Configuration

Test cases support the following configuration:

- **name**: Descriptive test name (displayed in output)
- **program**: Pointer to embedded binary data
- **program_size**: Size of binary in bytes
- **load_addr**: Memory address to load program (typically 0x1000)
- **max_steps**: Maximum execution steps before timeout (prevents infinite loops)
- **test_fn**: Validation function pointer

## Troubleshooting

### Test Times Out
- Check for infinite loops in assembly
- Increase `max_steps` in test case definition
- Verify SYSCALL is reached

### Assertion Failures
- Use `hexdump -C test.bin` to inspect binary
- Add debug output in validation function
- Verify instruction encoding matches specification

### Compilation Errors
- Ensure `.h` files are generated (run `make unit_test_headers`)
- Check that test is included in `test_suite.c`
- Verify test name matches across files

## Architecture

```
tests/unit/
├── test_*.asm           # Assembly test programs
├── test_*.bin           # Generated binaries (git ignored)
└── test_*.h             # Generated C headers (git ignored)

src/test/
├── test_runner.c        # Test execution engine
└── test_suite.c         # Test definitions and validations

include/
└── c32_test.h           # Test framework API
```

## Current Test Coverage

- ✅ Arithmetic: ADD, ADDI, SUB, MUL
- ✅ Logical: AND, OR, XOR
- ✅ Shifts: SLL, SRL
- ✅ Branches: BEQ
- ✅ Memory: LW, SW, LUI, ORI

## Future Enhancements

- Test fixtures for complex setups
- Performance benchmarking
- Code coverage analysis
- Randomized test generation
- Continuous integration hooks
