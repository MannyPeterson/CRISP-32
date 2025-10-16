/**
 * @file c32_asm.h
 * @brief CRISP-32 Two-Pass Assembler API
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

#ifndef C32_ASM_H
#define C32_ASM_H

#include "c32_types.h"

/**
 * @defgroup assembler CRISP-32 Assembler
 * @brief Two-pass assembler for CRISP-32 assembly language
 *
 * This module provides a complete two-pass assembler that translates
 * CRISP-32 assembly language into binary machine code. The assembler
 * supports labels, comments, and all CRISP-32 instructions defined
 * in the ISA specification.
 *
 * **Assembly Process:**
 * - Pass 1: Collect label definitions and calculate addresses
 * - Pass 2: Generate machine code with resolved symbols
 *
 * **Supported Features:**
 * - Labels and forward references
 * - All CRISP-32 instruction mnemonics
 * - Register names (R0-R31) and ABI names (zero, at, v0, etc.)
 * - Immediate values (decimal, hexadecimal)
 * - Comments (# or ; to end of line)
 *
 * @{
 */

/**
 * @defgroup asm_constants Assembler Constants
 * @brief Size limits and configuration constants
 * @{
 */

/** @brief Maximum number of symbols in symbol table */
#define MAX_SYMBOLS     1024

/** @brief Maximum length of input line */
#define MAX_LINE_LEN    256

/** @brief Maximum length of label name */
#define MAX_LABEL_LEN   64

/** @brief Maximum output binary size (64KB) */
#define MAX_OUTPUT_SIZE (64 * 1024)

/** @} */ /* end of asm_constants */

/**
 * @defgroup asm_types Assembler Data Types
 * @brief Core data structures for assembler state
 * @{
 */

/**
 * @brief Instruction encoding structure
 *
 * Represents a decoded CRISP-32 instruction with all fields separated.
 * All CRISP-32 instructions use 64-bit uniform encoding.
 */
typedef struct {
    uint8_t opcode;         /**< Operation code (0-255) */
    uint8_t rs;             /**< Source register 1 (0-31) */
    uint8_t rt;             /**< Source register 2 or target for immediates (0-31) */
    uint8_t rd;             /**< Destination register (0-31) */
    uint32_t immediate;     /**< 32-bit immediate value or offset */
} c32_instruction_t;

/**
 * @brief Symbol table entry
 *
 * Represents a label in the assembly program. Labels are resolved
 * during the first pass and used to calculate addresses in the second pass.
 */
typedef struct {
    char name[MAX_LABEL_LEN];   /**< Symbol name (label) */
    uint32_t address;           /**< Resolved address */
    int defined;                /**< 1 if defined, 0 if forward reference */
} c32_symbol_t;

/**
 * @brief Assembler state
 *
 * Complete state for the two-pass assembler including symbol table,
 * output buffer, and current assembly context.
 */
typedef struct {
    c32_symbol_t symbols[MAX_SYMBOLS];  /**< Symbol table */
    int num_symbols;                    /**< Number of symbols defined */
    uint32_t current_address;           /**< Current assembly address */
    uint8_t output[MAX_OUTPUT_SIZE];    /**< Output binary buffer */
    uint32_t output_size;               /**< Size of generated code in bytes */
    int pass;                           /**< Current pass (1 or 2) */
    int errors;                         /**< Number of assembly errors */
} c32_asm_state_t;

/** @} */ /* end of asm_types */

/**
 * @defgroup asm_api Assembler API Functions
 * @brief High-level assembler interface
 * @{
 */

/**
 * @brief Initialize assembler state
 *
 * Resets all fields to initial values. Must be called before
 * starting assembly.
 *
 * @param state Pointer to assembler state structure
 */
void c32_asm_init(c32_asm_state_t *state);

/**
 * @brief Assemble a source file
 *
 * Performs two-pass assembly on the input file and writes the
 * resulting binary to the output file.
 *
 * **Pass 1:** Scans the file to collect label definitions and
 * calculate instruction addresses.
 *
 * **Pass 2:** Generates machine code with all symbols resolved.
 *
 * @param state Pointer to assembler state
 * @param input_file Path to assembly source file (.asm)
 * @param output_file Path to output binary file (.bin)
 * @return 0 on success, -1 on error
 */
int c32_asm_assemble_file(c32_asm_state_t *state, const char *input_file, const char *output_file);

/**
 * @brief Assemble a single line
 *
 * Processes one line of assembly code. Behavior depends on current pass:
 * - **Pass 1:** Collects labels, validates syntax
 * - **Pass 2:** Generates machine code
 *
 * @param state Pointer to assembler state
 * @param line Assembly source line
 * @param line_num Line number (for error reporting)
 * @return 0 on success, -1 on error
 */
int c32_asm_assemble_line(c32_asm_state_t *state, const char *line, int line_num);

/** @} */ /* end of asm_api */

/**
 * @defgroup asm_symbols Symbol Table Functions
 * @brief Symbol table management for labels
 * @{
 */

/**
 * @brief Add symbol to symbol table
 *
 * Adds a new symbol (label) to the symbol table with the specified address.
 * If the symbol already exists, returns an error.
 *
 * @param state Pointer to assembler state
 * @param name Symbol name (label)
 * @param address Address associated with symbol
 * @return Symbol index on success, -1 on error (duplicate or table full)
 */
int c32_asm_add_symbol(c32_asm_state_t *state, const char *name, uint32_t address);

/**
 * @brief Find symbol in symbol table
 *
 * Searches the symbol table for a symbol with the given name.
 *
 * @param state Pointer to assembler state
 * @param name Symbol name to search for
 * @return Symbol index if found, -1 if not found
 */
int c32_asm_find_symbol(c32_asm_state_t *state, const char *name);

/** @} */ /* end of asm_symbols */

/**
 * @defgroup asm_encoding Instruction Encoding
 * @brief Low-level instruction encoding functions
 * @{
 */

/**
 * @brief Encode instruction to binary
 *
 * Converts a decoded instruction structure into 8 bytes of
 * little-endian machine code following the CRISP-32 instruction format:
 * - Byte 0: opcode
 * - Byte 1: rs
 * - Byte 2: rt
 * - Byte 3: rd
 * - Bytes 4-7: immediate (little-endian)
 *
 * @param output Pointer to 8-byte output buffer
 * @param inst Pointer to instruction structure
 */
void c32_encode_instruction(uint8_t *output, const c32_instruction_t *inst);

/** @} */ /* end of asm_encoding */

/**
 * @defgroup asm_parsing Parsing Utilities
 * @brief Helper functions for parsing assembly syntax
 * @{
 */

/**
 * @brief Parse register name or number
 *
 * Parses a register specifier and returns the register number (0-31).
 * Accepts both numeric form (R0-R31) and ABI names (zero, at, v0-v1,
 * a0-a3, t0-t9, s0-s7, k0-k1, gp, sp, fp, ra).
 *
 * **Examples:**
 * - "R0" or "zero" -> 0
 * - "R29" or "sp" -> 29
 * - "R31" or "ra" -> 31
 *
 * @param str Register string to parse
 * @return Register number (0-31) on success, -1 on error
 */
int c32_parse_register(const char *str);

/** @} */ /* end of asm_parsing */

/** @} */ /* end of assembler */

#endif /* C32_ASM_H */
