# Unit Test: Load/Store instructions (LW, SW)
# Expected results:
#   R1 = 0x12345678
#   Memory[0x2000] = 0x12345678

start:
    LUI R1, 0x1234      # R1 = 0x12340000
    ORI R1, R1, 0x5678  # R1 = 0x12345678
    SW R1, R0, 0x2000   # Memory[0x2000] = R1
    ADDI R2, R0, 0      # R2 = 0
    LW R2, R0, 0x2000   # R2 = Memory[0x2000]
    SYSCALL             # Halt
