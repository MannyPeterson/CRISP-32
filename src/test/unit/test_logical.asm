# Unit Test: Logical operations (AND, OR, XOR)
# Expected results:
#   R1 = 0x0F (15)
#   R2 = 0x33 (51)
#   R3 = 0x03 (R1 AND R2)
#   R4 = 0x3F (R1 OR R2)
#   R5 = 0x3C (R1 XOR R2)

start:
    ADDI R1, R0, 15     # R1 = 0x0F
    ADDI R2, R0, 51     # R2 = 0x33
    AND R3, R1, R2      # R3 = 0x0F & 0x33 = 0x03
    OR R4, R1, R2       # R4 = 0x0F | 0x33 = 0x3F
    XOR R5, R1, R2      # R5 = 0x0F ^ 0x33 = 0x3C
    SYSCALL             # Halt
