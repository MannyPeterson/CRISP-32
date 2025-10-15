/*
 * CRISP-32 Assembly Parser
 */

#include "c32_asm.h"
#include "c32_opcodes.h"
#include "c32_string.h"

/* Helper: skip whitespace */
static const char* skip_whitespace(const char *str) {
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

/* Helper: parse signed 32-bit immediate value */
static int parse_immediate(const char *str, int32_t *result) {
    int negative = 0;
    int32_t value = 0;
    const char *p = str;

    if (*p == '-') {
        negative = 1;
        p++;
    } else if (*p == '+') {
        p++;
    }

    /* Parse hex (0x prefix) */
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        while (*p) {
            if (*p >= '0' && *p <= '9') {
                value = (value << 4) | (*p - '0');
            } else if (*p >= 'a' && *p <= 'f') {
                value = (value << 4) | (*p - 'a' + 10);
            } else if (*p >= 'A' && *p <= 'F') {
                value = (value << 4) | (*p - 'A' + 10);
            } else {
                break;
            }
            p++;
        }
    } else {
        /* Parse decimal */
        while (*p >= '0' && *p <= '9') {
            value = value * 10 + (*p - '0');
            p++;
        }
    }

    *result = negative ? -value : value;
    return 0;
}

/* Parse instruction opcode and return opcode byte */
static int parse_opcode(const char *mnemonic) {
    /* Arithmetic */
    if (c32_strcmp(mnemonic, "ADD") == 0) return OP_ADD;
    if (c32_strcmp(mnemonic, "ADDU") == 0) return OP_ADDU;
    if (c32_strcmp(mnemonic, "SUB") == 0) return OP_SUB;
    if (c32_strcmp(mnemonic, "SUBU") == 0) return OP_SUBU;
    if (c32_strcmp(mnemonic, "ADDI") == 0) return OP_ADDI;
    if (c32_strcmp(mnemonic, "ADDIU") == 0) return OP_ADDIU;

    /* Logical */
    if (c32_strcmp(mnemonic, "AND") == 0) return OP_AND;
    if (c32_strcmp(mnemonic, "OR") == 0) return OP_OR;
    if (c32_strcmp(mnemonic, "XOR") == 0) return OP_XOR;
    if (c32_strcmp(mnemonic, "NOR") == 0) return OP_NOR;
    if (c32_strcmp(mnemonic, "ANDI") == 0) return OP_ANDI;
    if (c32_strcmp(mnemonic, "ORI") == 0) return OP_ORI;
    if (c32_strcmp(mnemonic, "XORI") == 0) return OP_XORI;
    if (c32_strcmp(mnemonic, "LUI") == 0) return OP_LUI;

    /* Shifts */
    if (c32_strcmp(mnemonic, "SLL") == 0) return OP_SLL;
    if (c32_strcmp(mnemonic, "SRL") == 0) return OP_SRL;
    if (c32_strcmp(mnemonic, "SRA") == 0) return OP_SRA;
    if (c32_strcmp(mnemonic, "SLLV") == 0) return OP_SLLV;
    if (c32_strcmp(mnemonic, "SRLV") == 0) return OP_SRLV;
    if (c32_strcmp(mnemonic, "SRAV") == 0) return OP_SRAV;

    /* Comparison */
    if (c32_strcmp(mnemonic, "SLT") == 0) return OP_SLT;
    if (c32_strcmp(mnemonic, "SLTU") == 0) return OP_SLTU;
    if (c32_strcmp(mnemonic, "SLTI") == 0) return OP_SLTI;
    if (c32_strcmp(mnemonic, "SLTIU") == 0) return OP_SLTIU;

    /* Multiply/Divide */
    if (c32_strcmp(mnemonic, "MUL") == 0) return OP_MUL;
    if (c32_strcmp(mnemonic, "MULH") == 0) return OP_MULH;
    if (c32_strcmp(mnemonic, "MULHU") == 0) return OP_MULHU;
    if (c32_strcmp(mnemonic, "DIV") == 0) return OP_DIV;
    if (c32_strcmp(mnemonic, "DIVU") == 0) return OP_DIVU;
    if (c32_strcmp(mnemonic, "REM") == 0) return OP_REM;
    if (c32_strcmp(mnemonic, "REMU") == 0) return OP_REMU;

    /* Load/Store */
    if (c32_strcmp(mnemonic, "LW") == 0) return OP_LW;
    if (c32_strcmp(mnemonic, "LH") == 0) return OP_LH;
    if (c32_strcmp(mnemonic, "LHU") == 0) return OP_LHU;
    if (c32_strcmp(mnemonic, "LB") == 0) return OP_LB;
    if (c32_strcmp(mnemonic, "LBU") == 0) return OP_LBU;
    if (c32_strcmp(mnemonic, "SW") == 0) return OP_SW;
    if (c32_strcmp(mnemonic, "SH") == 0) return OP_SH;
    if (c32_strcmp(mnemonic, "SB") == 0) return OP_SB;

    /* Branches */
    if (c32_strcmp(mnemonic, "BEQ") == 0) return OP_BEQ;
    if (c32_strcmp(mnemonic, "BNE") == 0) return OP_BNE;
    if (c32_strcmp(mnemonic, "BLEZ") == 0) return OP_BLEZ;
    if (c32_strcmp(mnemonic, "BGTZ") == 0) return OP_BGTZ;
    if (c32_strcmp(mnemonic, "BLTZ") == 0) return OP_BLTZ;
    if (c32_strcmp(mnemonic, "BGEZ") == 0) return OP_BGEZ;

    /* Jumps */
    if (c32_strcmp(mnemonic, "J") == 0) return OP_J;
    if (c32_strcmp(mnemonic, "JAL") == 0) return OP_JAL;
    if (c32_strcmp(mnemonic, "JR") == 0) return OP_JR;
    if (c32_strcmp(mnemonic, "JALR") == 0) return OP_JALR;

    /* System */
    if (c32_strcmp(mnemonic, "SYSCALL") == 0) return OP_SYSCALL;
    if (c32_strcmp(mnemonic, "BREAK") == 0) return OP_BREAK;
    if (c32_strcmp(mnemonic, "NOP") == 0) return OP_NOP;

    /* Interrupt */
    if (c32_strcmp(mnemonic, "EI") == 0) return OP_EI;
    if (c32_strcmp(mnemonic, "DI") == 0) return OP_DI;
    if (c32_strcmp(mnemonic, "IRET") == 0) return OP_IRET;
    if (c32_strcmp(mnemonic, "RAISE") == 0) return OP_RAISE;
    if (c32_strcmp(mnemonic, "GETPC") == 0) return OP_GETPC;

    /* MMU */
    if (c32_strcmp(mnemonic, "ENABLE_PAGING") == 0) return OP_ENABLE_PAGING;
    if (c32_strcmp(mnemonic, "DISABLE_PAGING") == 0) return OP_DISABLE_PAGING;
    if (c32_strcmp(mnemonic, "SET_PTBR") == 0) return OP_SET_PTBR;
    if (c32_strcmp(mnemonic, "ENTER_USER") == 0) return OP_ENTER_USER;
    if (c32_strcmp(mnemonic, "GETMODE") == 0) return OP_GETMODE;

    return -1; /* Unknown instruction */
}

/* Tokenize a line into tokens separated by whitespace and commas */
static int tokenize(const char *line, char tokens[][MAX_LABEL_LEN], int max_tokens) {
    int token_count = 0;
    int token_pos = 0;
    const char *p = line;

    while (*p && token_count < max_tokens) {
        p = skip_whitespace(p);
        if (*p == '\0' || *p == ';' || *p == '#') {
            break; /* End of line or comment */
        }

        /* Read token until whitespace or comma */
        token_pos = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != ',' && *p != ';' && *p != '#') {
            if (token_pos < MAX_LABEL_LEN - 1) {
                tokens[token_count][token_pos++] = *p;
            }
            p++;
        }
        tokens[token_count][token_pos] = '\0';

        if (token_pos > 0) {
            token_count++;
        }

        /* Skip comma */
        if (*p == ',') {
            p++;
        }
    }

    return token_count;
}

/* Assemble a single line */
int c32_asm_assemble_line(c32_asm_state_t *state, const char *line, int line_num) {
    char tokens[8][MAX_LABEL_LEN];
    int token_count;
    c32_instruction_t inst;
    int opcode;
    int32_t imm;
    int idx;

    (void)line_num; /* Unused for now */

    if (!state || !line) {
        return -1;
    }

    /* Skip empty lines and comments */
    line = skip_whitespace(line);
    if (*line == '\0' || *line == ';' || *line == '#') {
        return 0;
    }

    /* Check for label (ends with :) */
    {
        size_t len = 0;
        const char *p = line;
        while (*p && *p != ':' && *p != ' ' && *p != '\t') {
            len++;
            p++;
        }
        if (*p == ':' && len > 0) {
            char label[MAX_LABEL_LEN];
            if (len >= MAX_LABEL_LEN) {
                return -1;
            }
            c32_memcpy(label, line, len);
            label[len] = '\0';

            /* Add label to symbol table in pass 1 */
            if (state->pass == 1) {
                c32_asm_add_symbol(state, label, state->current_address);
            }

            /* Skip past label for rest of parsing */
            line = p + 1;
            line = skip_whitespace(line);
            if (*line == '\0') {
                return 0; /* Just a label, no instruction */
            }
        }
    }

    /* Tokenize the line */
    token_count = tokenize(line, tokens, 8);
    if (token_count == 0) {
        return 0;
    }

    /* Get opcode */
    opcode = parse_opcode(tokens[0]);
    if (opcode < 0) {
        return -1; /* Unknown instruction */
    }

    /* Initialize instruction */
    c32_memset(&inst, 0, sizeof(inst));
    inst.opcode = (uint8_t)opcode;

    /* Parse operands based on instruction type */
    /* R-type: ADD rd, rs, rt */
    if (opcode == OP_ADD || opcode == OP_ADDU || opcode == OP_SUB || opcode == OP_SUBU ||
        opcode == OP_AND || opcode == OP_OR || opcode == OP_XOR || opcode == OP_NOR ||
        opcode == OP_SLT || opcode == OP_SLTU ||
        opcode == OP_MUL || opcode == OP_MULH || opcode == OP_MULHU ||
        opcode == OP_DIV || opcode == OP_DIVU || opcode == OP_REM || opcode == OP_REMU ||
        opcode == OP_SLLV || opcode == OP_SRLV || opcode == OP_SRAV) {
        if (token_count < 4) return -1;
        inst.rd = (uint8_t)c32_parse_register(tokens[1]);
        inst.rs = (uint8_t)c32_parse_register(tokens[2]);
        inst.rt = (uint8_t)c32_parse_register(tokens[3]);
        inst.immediate = 0;
    }
    /* I-type: ADDI rt, rs, imm */
    else if (opcode == OP_ADDI || opcode == OP_ADDIU ||
             opcode == OP_ANDI || opcode == OP_ORI || opcode == OP_XORI ||
             opcode == OP_SLTI || opcode == OP_SLTIU) {
        if (token_count < 4) return -1;
        inst.rt = (uint8_t)c32_parse_register(tokens[1]);
        inst.rs = (uint8_t)c32_parse_register(tokens[2]);
        parse_immediate(tokens[3], &imm);
        inst.immediate = (uint32_t)imm;
    }
    /* LUI rt, imm */
    else if (opcode == OP_LUI) {
        if (token_count < 3) return -1;
        inst.rt = (uint8_t)c32_parse_register(tokens[1]);
        parse_immediate(tokens[2], &imm);
        inst.immediate = (uint32_t)imm;
    }
    /* Shift immediate: SLL rd, rt, shamt */
    else if (opcode == OP_SLL || opcode == OP_SRL || opcode == OP_SRA) {
        if (token_count < 4) return -1;
        inst.rd = (uint8_t)c32_parse_register(tokens[1]);
        inst.rt = (uint8_t)c32_parse_register(tokens[2]);
        parse_immediate(tokens[3], &imm);
        inst.immediate = (uint32_t)imm;
    }
    /* Branch: BEQ rs, rt, offset */
    else if (opcode == OP_BEQ || opcode == OP_BNE) {
        if (token_count < 4) return -1;
        inst.rs = (uint8_t)c32_parse_register(tokens[1]);
        inst.rt = (uint8_t)c32_parse_register(tokens[2]);
        /* Check if label or immediate */
        idx = c32_asm_find_symbol(state, tokens[3]);
        if (idx >= 0) {
            /* Label - calculate offset */
            inst.immediate = state->symbols[idx].address - (state->current_address + 8);
        } else {
            parse_immediate(tokens[3], &imm);
            inst.immediate = (uint32_t)imm;
        }
    }
    /* Branch single register: BLEZ rs, offset */
    else if (opcode == OP_BLEZ || opcode == OP_BGTZ || opcode == OP_BLTZ || opcode == OP_BGEZ) {
        if (token_count < 3) return -1;
        inst.rs = (uint8_t)c32_parse_register(tokens[1]);
        idx = c32_asm_find_symbol(state, tokens[2]);
        if (idx >= 0) {
            inst.immediate = state->symbols[idx].address - (state->current_address + 8);
        } else {
            parse_immediate(tokens[2], &imm);
            inst.immediate = (uint32_t)imm;
        }
    }
    /* Jump: J target or JAL target */
    else if (opcode == OP_J || opcode == OP_JAL) {
        if (token_count < 2) return -1;
        idx = c32_asm_find_symbol(state, tokens[1]);
        if (idx >= 0) {
            /* Add default load address (0x1000) to make addresses absolute */
            inst.immediate = state->symbols[idx].address + 0x1000;
        } else {
            parse_immediate(tokens[1], &imm);
            inst.immediate = (uint32_t)imm + 0x1000;
        }
    }
    /* JR rs or JALR rd, rs */
    else if (opcode == OP_JR) {
        if (token_count < 2) return -1;
        inst.rs = (uint8_t)c32_parse_register(tokens[1]);
    }
    else if (opcode == OP_JALR) {
        if (token_count < 3) return -1;
        inst.rd = (uint8_t)c32_parse_register(tokens[1]);
        inst.rs = (uint8_t)c32_parse_register(tokens[2]);
    }
    /* Load/Store: LW rt, rs, offset or SW rt, rs, offset */
    else if (opcode == OP_LW || opcode == OP_LH || opcode == OP_LHU ||
             opcode == OP_LB || opcode == OP_LBU ||
             opcode == OP_SW || opcode == OP_SH || opcode == OP_SB) {
        if (token_count < 4) return -1;
        inst.rt = (uint8_t)c32_parse_register(tokens[1]);
        inst.rs = (uint8_t)c32_parse_register(tokens[2]);
        parse_immediate(tokens[3], &imm);
        inst.immediate = (uint32_t)imm;
    }
    /* No operands: NOP, EI, DI, IRET, SYSCALL, BREAK, etc. */
    else {
        /* Already initialized to zeros */
    }

    /* Encode instruction in pass 2 */
    if (state->pass == 2) {
        if (state->output_size + 8 > MAX_OUTPUT_SIZE) {
            return -1;
        }
        c32_encode_instruction(state->output + state->output_size, &inst);
        state->output_size += 8;
    }

    /* Advance address */
    state->current_address += 8;

    return 0;
}
