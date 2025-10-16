# Unit Test: Shift operations (SLL, SRL)
# Expected results:
#   R1 = 8
#   R2 = 32 (8 << 2)
#   R3 = 4  (8 >> 1)

start:
    ADDI R1, R0, 8      # R1 = 8
    SLL R2, R1, 2       # R2 = R1 << 2 = 32
    SRL R3, R1, 1       # R3 = R1 >> 1 = 4
    SYSCALL             # Halt
