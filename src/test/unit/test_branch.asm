# Unit Test: Branch instructions (BEQ, BNE)
# Expected results:
#   R1 = 5
#   R2 = 5
#   R3 = 1 (branch was taken)

start:
    ADDI R1, R0, 5      # R1 = 5
    ADDI R2, R0, 5      # R2 = 5
    ADDI R3, R0, 0      # R3 = 0
    BEQ R1, R2, taken   # Should branch (R1 == R2)
    ADDI R3, R0, 99     # Should not execute
    SYSCALL

taken:
    ADDI R3, R0, 1      # R3 = 1 (indicates branch was taken)
    SYSCALL             # Halt
