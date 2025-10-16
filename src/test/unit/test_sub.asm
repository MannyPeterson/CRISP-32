# Unit Test: SUB instruction
# Expected results:
#   R1 = 100
#   R2 = 30
#   R3 = 70

start:
    ADDI R1, R0, 100    # R1 = 100
    ADDI R2, R0, 30     # R2 = 30
    SUB R3, R1, R2      # R3 = R1 - R2 = 70
    SYSCALL             # Halt
