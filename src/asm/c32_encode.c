/*
 * CRISP-32 Instruction Encoding
 */

#include "c32_asm.h"
#include "c32_vm.h"
#include "c32_string.h"

/* Encode a CRISP-32 instruction to 8 bytes (little-endian) */
void c32_encode_instruction(uint8_t *output, const c32_instruction_t *inst) {
    /* Format: [opcode][rs][rt][rd][immediate] */
    /* Byte 0: opcode */
    output[0] = inst->opcode;

    /* Byte 1: rs */
    output[1] = inst->rs;

    /* Byte 2: rt */
    output[2] = inst->rt;

    /* Byte 3: rd */
    output[3] = inst->rd;

    /* Bytes 4-7: immediate (32-bit, little-endian) */
    c32_write_word(output + 4, inst->immediate);
}

/* Parse a register name like "R5", "r10", "zero", "sp", etc. */
int c32_parse_register(const char *str) {
    int reg;
    size_t len;

    if (!str) {
        return -1;
    }

    len = c32_strlen(str);
    if (len == 0) {
        return -1;
    }

    /* Handle special register names */
    if (c32_strcmp(str, "zero") == 0) return 0;
    if (c32_strcmp(str, "at") == 0) return 1;
    if (c32_strcmp(str, "v0") == 0) return 2;
    if (c32_strcmp(str, "v1") == 0) return 3;
    if (c32_strcmp(str, "a0") == 0) return 4;
    if (c32_strcmp(str, "a1") == 0) return 5;
    if (c32_strcmp(str, "a2") == 0) return 6;
    if (c32_strcmp(str, "a3") == 0) return 7;
    if (c32_strcmp(str, "t0") == 0) return 8;
    if (c32_strcmp(str, "t1") == 0) return 9;
    if (c32_strcmp(str, "t2") == 0) return 10;
    if (c32_strcmp(str, "t3") == 0) return 11;
    if (c32_strcmp(str, "t4") == 0) return 12;
    if (c32_strcmp(str, "t5") == 0) return 13;
    if (c32_strcmp(str, "t6") == 0) return 14;
    if (c32_strcmp(str, "t7") == 0) return 15;
    if (c32_strcmp(str, "s0") == 0) return 16;
    if (c32_strcmp(str, "s1") == 0) return 17;
    if (c32_strcmp(str, "s2") == 0) return 18;
    if (c32_strcmp(str, "s3") == 0) return 19;
    if (c32_strcmp(str, "s4") == 0) return 20;
    if (c32_strcmp(str, "s5") == 0) return 21;
    if (c32_strcmp(str, "s6") == 0) return 22;
    if (c32_strcmp(str, "s7") == 0) return 23;
    if (c32_strcmp(str, "t8") == 0) return 24;
    if (c32_strcmp(str, "t9") == 0) return 25;
    if (c32_strcmp(str, "k0") == 0) return 26;
    if (c32_strcmp(str, "k1") == 0) return 27;
    if (c32_strcmp(str, "gp") == 0) return 28;
    if (c32_strcmp(str, "sp") == 0) return 29;
    if (c32_strcmp(str, "fp") == 0) return 30;
    if (c32_strcmp(str, "ra") == 0) return 31;

    /* Handle R## or r## format */
    if ((str[0] == 'R' || str[0] == 'r') && len > 1) {
        reg = 0;
        /* Simple atoi for register number */
        {
            size_t i;
            for (i = 1; i < len; i++) {
                if (str[i] >= '0' && str[i] <= '9') {
                    reg = reg * 10 + (str[i] - '0');
                } else {
                    return -1;
                }
            }
        }

        if (reg >= 0 && reg <= 31) {
            return reg;
        }
    }

    return -1;
}
