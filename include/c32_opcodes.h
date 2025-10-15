/**
 * @file c32_opcodes.h
 * @brief CRISP-32 Instruction Opcode Definitions
 * @author Manny Peterson <manny@manny.ca>
 * @date 2025
 * @copyright Copyright (C) 2025 Manny Peterson
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef C32_OPCODES_H
#define C32_OPCODES_H

/**
 * @defgroup opcodes Instruction Opcodes
 * @brief All instruction opcodes from the CRISP-32 ISA specification
 *
 * Complete set of 80+ instruction opcodes organized by category.
 * Each instruction is 64 bits (8 bytes) with uniform encoding.
 * @{
 */

/**
 * @defgroup opcode_special Special Operations
 * @brief No operation and reserved opcodes
 * @{
 */

/** @brief No operation */
#define OP_NOP          0x00

/** @} */ /* end of opcode_special */

/**
 * @defgroup opcode_arithmetic Arithmetic Operations
 * @brief Integer arithmetic (add, subtract, multiply, divide)
 * @{
 */

/** @brief Add signed (rd = rs + rt) */
#define OP_ADD          0x01

/** @brief Add unsigned (rd = rs + rt) */
#define OP_ADDU         0x02

/** @brief Subtract signed (rd = rs - rt) */
#define OP_SUB          0x03

/** @brief Subtract unsigned (rd = rs - rt) */
#define OP_SUBU         0x04

/** @brief Add immediate signed (rt = rs + imm) */
#define OP_ADDI         0x05

/** @brief Add immediate unsigned (rt = rs + imm) */
#define OP_ADDIU        0x06

/** @brief Multiply lower 32 bits (rd = (rs * rt)[31:0]) */
#define OP_MUL          0x40

/** @brief Multiply high signed (rd = (rs * rt)[63:32]) */
#define OP_MULH         0x41

/** @brief Multiply high unsigned (rd = (rs * rt)[63:32]) */
#define OP_MULHU        0x42

/** @brief Divide signed (rd = rs / rt) */
#define OP_DIV          0x43

/** @brief Divide unsigned (rd = rs / rt) */
#define OP_DIVU         0x44

/** @brief Remainder signed (rd = rs % rt) */
#define OP_REM          0x45

/** @brief Remainder unsigned (rd = rs % rt) */
#define OP_REMU         0x46

/** @} */ /* end of opcode_arithmetic */

/**
 * @defgroup opcode_logical Logical Operations
 * @brief Bitwise logical operations (AND, OR, XOR, NOR)
 * @{
 */

/** @brief Bitwise AND (rd = rs & rt) */
#define OP_AND          0x10

/** @brief Bitwise OR (rd = rs | rt) */
#define OP_OR           0x11

/** @brief Bitwise XOR (rd = rs ^ rt) */
#define OP_XOR          0x12

/** @brief Bitwise NOR (rd = ~(rs | rt)) */
#define OP_NOR          0x13

/** @brief AND immediate (rt = rs & imm) */
#define OP_ANDI         0x14

/** @brief OR immediate (rt = rs | imm) */
#define OP_ORI          0x15

/** @brief XOR immediate (rt = rs ^ imm) */
#define OP_XORI         0x16

/** @brief Load upper immediate (rt = imm << 16) */
#define OP_LUI          0x17

/** @} */ /* end of opcode_logical */

/**
 * @defgroup opcode_shift Shift Operations
 * @brief Logical and arithmetic shifts
 * @{
 */

/** @brief Shift left logical (rd = rt << imm) */
#define OP_SLL          0x20

/** @brief Shift right logical (rd = rt >> imm) */
#define OP_SRL          0x21

/** @brief Shift right arithmetic (rd = rt >> imm, sign extend) */
#define OP_SRA          0x22

/** @brief Shift left logical variable (rd = rt << rs) */
#define OP_SLLV         0x23

/** @brief Shift right logical variable (rd = rt >> rs) */
#define OP_SRLV         0x24

/** @brief Shift right arithmetic variable (rd = rt >> rs, sign extend) */
#define OP_SRAV         0x25

/** @} */ /* end of opcode_shift */

/**
 * @defgroup opcode_comparison Comparison Operations
 * @brief Set-on-less-than operations
 * @{
 */

/** @brief Set on less than signed (rd = (rs < rt) ? 1 : 0) */
#define OP_SLT          0x30

/** @brief Set on less than unsigned (rd = (rs < rt) ? 1 : 0) */
#define OP_SLTU         0x31

/** @brief Set on less than immediate signed (rt = (rs < imm) ? 1 : 0) */
#define OP_SLTI         0x32

/** @brief Set on less than immediate unsigned (rt = (rs < imm) ? 1 : 0) */
#define OP_SLTIU        0x33

/** @} */ /* end of opcode_comparison */

/**
 * @defgroup opcode_memory Memory Operations
 * @brief Load and store instructions
 * @{
 */

/** @brief Load word (rt = mem[rs+offset]) */
#define OP_LW           0x50

/** @brief Load halfword signed (rt = sign_ext(mem[rs+offset])) */
#define OP_LH           0x51

/** @brief Load halfword unsigned (rt = zero_ext(mem[rs+offset])) */
#define OP_LHU          0x52

/** @brief Load byte signed (rt = sign_ext(mem[rs+offset])) */
#define OP_LB           0x53

/** @brief Load byte unsigned (rt = zero_ext(mem[rs+offset])) */
#define OP_LBU          0x54

/** @brief Store word (mem[rs+offset] = rt) */
#define OP_SW           0x58

/** @brief Store halfword (mem[rs+offset] = rt[15:0]) */
#define OP_SH           0x59

/** @brief Store byte (mem[rs+offset] = rt[7:0]) */
#define OP_SB           0x5A

/** @} */ /* end of opcode_memory */

/**
 * @defgroup opcode_branch Branch Operations
 * @brief Conditional branch instructions
 * @{
 */

/** @brief Branch if equal (if (rs == rt) pc += offset) */
#define OP_BEQ          0x60

/** @brief Branch if not equal (if (rs != rt) pc += offset) */
#define OP_BNE          0x61

/** @brief Branch if less than or equal to zero (if (rs <= 0) pc += offset) */
#define OP_BLEZ         0x62

/** @brief Branch if greater than zero (if (rs > 0) pc += offset) */
#define OP_BGTZ         0x63

/** @brief Branch if less than zero (if (rs < 0) pc += offset) */
#define OP_BLTZ         0x64

/** @brief Branch if greater than or equal to zero (if (rs >= 0) pc += offset) */
#define OP_BGEZ         0x65

/** @} */ /* end of opcode_branch */

/**
 * @defgroup opcode_jump Jump Operations
 * @brief Unconditional jump instructions
 * @{
 */

/** @brief Jump (pc = target) */
#define OP_J            0x70

/** @brief Jump and link (r31 = pc+8; pc = target) */
#define OP_JAL          0x71

/** @brief Jump register (pc = rs) */
#define OP_JR           0x72

/** @brief Jump and link register (rd = pc+8; pc = rs) */
#define OP_JALR         0x73

/** @} */ /* end of opcode_jump */

/**
 * @defgroup opcode_system System Operations
 * @brief System calls and breakpoints
 * @{
 */

/** @brief System call (raises interrupt 4) */
#define OP_SYSCALL      0xF0

/** @brief Breakpoint (raises interrupt 5) */
#define OP_BREAK        0xF1

/** @} */ /* end of opcode_system */

/**
 * @defgroup opcode_interrupt Interrupt Control
 * @brief Interrupt enable/disable and return
 * @{
 */

/** @brief Enable interrupts (privileged) */
#define OP_EI           0xF2

/** @brief Disable interrupts (privileged) */
#define OP_DI           0xF3

/** @brief Return from interrupt (restore context, privileged) */
#define OP_IRET         0xF4

/** @brief Raise software interrupt (set pending bit) */
#define OP_RAISE        0xF5

/** @brief Get saved PC from interrupt (rd = saved_pc) */
#define OP_GETPC        0xF6

/** @} */ /* end of opcode_interrupt */

/**
 * @defgroup opcode_privilege Privilege and MMU Control
 * @brief Memory protection and privilege level management
 * @{
 */

/** @brief Enable paging/virtual addressing (privileged) */
#define OP_ENABLE_PAGING  0xF7

/** @brief Disable paging/virtual addressing (privileged) */
#define OP_DISABLE_PAGING 0xF8

/** @brief Set page table base register (rd=base, rt=num_pages, privileged) */
#define OP_SET_PTBR     0xF9

/** @brief Enter user mode (drop to user privilege, privileged) */
#define OP_ENTER_USER   0xFB

/** @brief Get current mode (rd = kernel_mode flag) */
#define OP_GETMODE      0xFC

/** @} */ /* end of opcode_privilege */

/** @} */ /* end of opcodes */

#endif /* C32_OPCODES_H */
