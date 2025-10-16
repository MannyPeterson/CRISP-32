# Test: Jump and Function Call Instructions
# Tests: JAL, JR, J
# Expected Results:
#   R1 = 101 (100 from function + 1 after return)
#   R2 = non-zero (saved return address from JAL)
#   R3 = 50 (set after J instruction)

main:
    ADDI R1, R0, 0          # R1 = 0 (counter)
    JAL func1               # Call func1, R31 = return address
    ADDI R1, R1, 1          # R1++ (should execute after return, R1=101)
    J skip                  # Jump over next instruction

    ADDI R3, R0, 99         # Should NOT execute (jumped over)

skip:
    ADDI R3, R0, 50         # R3 = 50 (proves J worked)
    SYSCALL                 # Halt

func1:
    ADDI R2, R31, 0         # R2 = return address (save it)
    ADDI R1, R1, 100        # R1 += 100 (R1=100)
    JR R31                  # Return to caller
