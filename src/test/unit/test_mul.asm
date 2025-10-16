# Unit Test: MUL instruction
# Expected results:
#   R1 = 7
#   R2 = 6
#   R3 = 42

start:
    ADDI R1, R0, 7      # R1 = 7
    ADDI R2, R0, 6      # R2 = 6
    MUL R3, R1, R2      # R3 = R1 * R2 = 42
    SYSCALL             # Halt
