/*
 * CRISP-32 Opcode Definitions
 * All instruction opcodes from the ISA specification
 */

#ifndef C32_OPCODES_H
#define C32_OPCODES_H

/* Opcode definitions */
#define OP_NOP          0x00

/* Arithmetic - Register-Register */
#define OP_ADD          0x01
#define OP_ADDU         0x02
#define OP_SUB          0x03
#define OP_SUBU         0x04

/* Arithmetic - Register-Immediate */
#define OP_ADDI         0x05
#define OP_ADDIU        0x06

/* Logical - Register-Register */
#define OP_AND          0x10
#define OP_OR           0x11
#define OP_XOR          0x12
#define OP_NOR          0x13

/* Logical - Register-Immediate */
#define OP_ANDI         0x14
#define OP_ORI          0x15
#define OP_XORI         0x16
#define OP_LUI          0x17

/* Shift - Immediate Amount */
#define OP_SLL          0x20
#define OP_SRL          0x21
#define OP_SRA          0x22

/* Shift - Variable Amount */
#define OP_SLLV         0x23
#define OP_SRLV         0x24
#define OP_SRAV         0x25

/* Comparison - Register-Register */
#define OP_SLT          0x30
#define OP_SLTU         0x31

/* Comparison - Register-Immediate */
#define OP_SLTI         0x32
#define OP_SLTIU        0x33

/* Multiplication and Division */
#define OP_MUL          0x40
#define OP_MULH         0x41
#define OP_MULHU        0x42
#define OP_DIV          0x43
#define OP_DIVU         0x44
#define OP_REM          0x45
#define OP_REMU         0x46

/* Load Operations */
#define OP_LW           0x50
#define OP_LH           0x51
#define OP_LHU          0x52
#define OP_LB           0x53
#define OP_LBU          0x54

/* Store Operations */
#define OP_SW           0x58
#define OP_SH           0x59
#define OP_SB           0x5A

/* Branch Operations */
#define OP_BEQ          0x60
#define OP_BNE          0x61
#define OP_BLEZ         0x62
#define OP_BGTZ         0x63
#define OP_BLTZ         0x64
#define OP_BGEZ         0x65

/* Jump Operations */
#define OP_J            0x70
#define OP_JAL          0x71
#define OP_JR           0x72
#define OP_JALR         0x73

/* System Operations */
#define OP_SYSCALL      0xF0
#define OP_BREAK        0xF1

/* Interrupt Control */
#define OP_EI           0xF2
#define OP_DI           0xF3
#define OP_IRET         0xF4
#define OP_RAISE        0xF5
#define OP_GETPC        0xF6

/* Privilege and MMU Control */
#define OP_ENABLE_PAGING  0xF7
#define OP_DISABLE_PAGING 0xF8
#define OP_SET_PTBR     0xF9
#define OP_ENTER_USER   0xFB
#define OP_GETMODE      0xFC

#endif /* C32_OPCODES_H */
