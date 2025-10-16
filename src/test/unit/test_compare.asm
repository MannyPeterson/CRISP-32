# Test: Comparison Operations
# Tests: SLT, SLTU, SLTI, SLTIU
# Expected Results:
#   R3 = 1  (5 < 10, signed)
#   R4 = 0  (10 < 5 is false, signed)
#   R5 = 1  (5 < 20, signed immediate)
#   R7 = 1  (10 < 0xFFFFFFFF, unsigned)
#   R8 = 1  (5 < 100, unsigned immediate)

main:
    # Test SLT (set on less than, signed): 5 < 10 = true
    ADDI R1, R0, 5
    ADDI R2, R0, 10
    SLT R3, R1, R2          # R3 = 1 (5 < 10)

    # Test SLT (signed): 10 < 5 = false
    SLT R4, R2, R1          # R4 = 0 (10 >= 5)

    # Test SLTI (set on less than immediate, signed): 5 < 20 = true
    SLTI R5, R1, 20         # R5 = 1 (5 < 20)

    # Test SLTU (unsigned): 10 < 0xFFFFFFFF = true
    ADDI R6, R0, -1         # R6 = -1 = 0xFFFFFFFF
    SLTU R7, R2, R6         # R7 = 1 (10 < 4294967295 unsigned)

    # Test SLTIU (unsigned immediate): 5 < 100 = true
    SLTIU R8, R1, 100       # R8 = 1 (5 < 100)

    SYSCALL                 # Halt
