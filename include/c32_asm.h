/*
 * CRISP-32 Assembler
 * Two-pass assembler for CRISP-32 architecture
 */

#ifndef C32_ASM_H
#define C32_ASM_H

#include "c32_types.h"

/* Maximum sizes */
#define MAX_SYMBOLS     1024
#define MAX_LINE_LEN    256
#define MAX_LABEL_LEN   64
#define MAX_OUTPUT_SIZE (64 * 1024)  /* 64KB max output */

/* Instruction encoding structure */
typedef struct {
    uint8_t opcode;
    uint8_t rs;
    uint8_t rt;
    uint8_t rd;
    uint32_t immediate;
} c32_instruction_t;

/* Symbol table entry */
typedef struct {
    char name[MAX_LABEL_LEN];
    uint32_t address;
    int defined;
} c32_symbol_t;

/* Assembler state */
typedef struct {
    c32_symbol_t symbols[MAX_SYMBOLS];
    int num_symbols;
    uint32_t current_address;
    uint8_t output[MAX_OUTPUT_SIZE];
    uint32_t output_size;
    int pass;
    int errors;
} c32_asm_state_t;

/* Assembler API */
void c32_asm_init(c32_asm_state_t *state);
int c32_asm_assemble_file(c32_asm_state_t *state, const char *input_file, const char *output_file);
int c32_asm_assemble_line(c32_asm_state_t *state, const char *line, int line_num);

/* Symbol table functions */
int c32_asm_add_symbol(c32_asm_state_t *state, const char *name, uint32_t address);
int c32_asm_find_symbol(c32_asm_state_t *state, const char *name);

/* Instruction encoding */
void c32_encode_instruction(uint8_t *output, const c32_instruction_t *inst);

/* Register parsing */
int c32_parse_register(const char *str);

#endif /* C32_ASM_H */
