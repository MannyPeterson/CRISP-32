# Test: Branch Instruction Variants
# Tests: BNE, BLEZ, BGTZ, BLTZ, BGEZ
# Expected Results:
#   R3  = 1  (BNE taken)
#   R5  = 2  (BLEZ taken with zero)
#   R7  = 3  (BGTZ taken with positive)
#   R9  = 4  (BLTZ taken with negative)
#   R11 = 5  (BGEZ taken with zero)

main:
    # Test BNE (branch if not equal) - should branch
    ADDI R1, R0, 5
    ADDI R2, R0, 10
    ADDI R3, R0, 0          # R3 = 0 (will become 1)
    BNE R1, R2, bne_taken   # Branch because 5 != 10
    ADDI R3, R0, 99         # Should NOT execute

bne_taken:
    ADDI R3, R3, 1          # R3 = 1

    # Test BLEZ (branch if <= 0) - should branch with zero
    ADDI R4, R0, 0
    ADDI R5, R0, 0          # R5 = 0 (will become 2)
    BLEZ R4, blez_taken     # Branch because 0 <= 0
    ADDI R5, R0, 99         # Should NOT execute

blez_taken:
    ADDI R5, R5, 2          # R5 = 2

    # Test BGTZ (branch if > 0) - should branch with positive
    ADDI R6, R0, 10
    ADDI R7, R0, 0          # R7 = 0 (will become 3)
    BGTZ R6, bgtz_taken     # Branch because 10 > 0
    ADDI R7, R0, 99         # Should NOT execute

bgtz_taken:
    ADDI R7, R7, 3          # R7 = 3

    # Test BLTZ (branch if < 0) - should branch with negative
    ADDI R8, R0, -5         # R8 = -5
    ADDI R9, R0, 0          # R9 = 0 (will become 4)
    BLTZ R8, bltz_taken     # Branch because -5 < 0
    ADDI R9, R0, 99         # Should NOT execute

bltz_taken:
    ADDI R9, R9, 4          # R9 = 4

    # Test BGEZ (branch if >= 0) - should branch with zero
    ADDI R10, R0, 0
    ADDI R11, R0, 0         # R11 = 0 (will become 5)
    BGEZ R10, bgez_taken    # Branch because 0 >= 0
    ADDI R11, R0, 99        # Should NOT execute

bgez_taken:
    ADDI R11, R11, 5        # R11 = 5

    SYSCALL                 # Halt
