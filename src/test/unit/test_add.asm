# Unit Test: ADD and ADDI instructions
# Expected results:
#   R1 = 42
#   R2 = 10
#   R3 = 52

start:
    ADDI R1, R0, 42     # R1 = 42
    ADDI R2, R0, 10     # R2 = 10
    ADD R3, R1, R2      # R3 = R1 + R2 = 52
    SYSCALL             # Halt
