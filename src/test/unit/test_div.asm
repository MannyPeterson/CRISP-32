# Test: Division, Remainder, and Multiply High Operations
# Tests: DIV, DIVU, REM, REMU, MULH, MULHU
# Expected Results:
#   R3  = 14 (100 / 7, signed)
#   R4  = 2  (100 % 7, signed remainder)
#   R7  = 40 (1000 / 25, unsigned)
#   R8  = 0  (1000 % 25, unsigned remainder)
#   R10 = 1  (high 32 bits of 65536 * 65536, signed)
#   R11 = 1  (high 32 bits of 65536 * 65536, unsigned)

main:
    # Test DIV (signed division): 100 / 7 = 14
    ADDI R1, R0, 100
    ADDI R2, R0, 7
    DIV R3, R1, R2          # R3 = 14

    # Test REM (signed remainder): 100 % 7 = 2
    REM R4, R1, R2          # R4 = 2

    # Test DIVU (unsigned division): 1000 / 25 = 40
    ADDI R5, R0, 1000
    ADDI R6, R0, 25
    DIVU R7, R5, R6         # R7 = 40

    # Test REMU (unsigned remainder): 1000 % 25 = 0
    REMU R8, R5, R6         # R8 = 0

    # Test MULH (multiply high, signed)
    # 65536 * 65536 = 4294967296 = 0x1_00000000
    # Low 32 bits: 0x00000000, High 32 bits: 0x00000001
    LUI R9, 1               # R9 = 0x10000 = 65536
    MULH R10, R9, R9        # R10 = high 32 bits = 1

    # Test MULHU (multiply high, unsigned)
    MULHU R11, R9, R9       # R11 = 1

    SYSCALL                 # Halt
