/*
 * CRISP-32 Assembler - Main Program
 * Two-pass assembler
 *
 * Note: This tool uses stdio for file I/O (not freestanding)
 */

#include "c32_asm.h"
#include "c32_string.h"

/* We need file I/O, so include stdio */
#include <stdio.h>

/* Assemble a file */
int c32_asm_assemble_file(c32_asm_state_t *state, const char *input_file, const char *output_file) {
    FILE *input;
    FILE *output;
    char line[MAX_LINE_LEN];
    int line_num;

    if (!state || !input_file || !output_file) {
        return -1;
    }

    /* Pass 1: Collect labels and calculate addresses */
    state->pass = 1;
    state->current_address = 0;
    state->output_size = 0;

    input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", input_file);
        return -1;
    }

    line_num = 0;
    while (fgets(line, MAX_LINE_LEN, input)) {
        size_t len;
        line_num++;

        /* Strip newline */
        len = c32_strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        if (len > 1 && line[len-2] == '\r') {
            line[len-2] = '\0';
        }

        if (c32_asm_assemble_line(state, line, line_num) < 0) {
            fprintf(stderr, "Error: Pass 1, line %d: %s\n", line_num, line);
            state->errors++;
        }
    }

    fclose(input);

    if (state->errors > 0) {
        fprintf(stderr, "Pass 1 failed with %d errors\n", state->errors);
        return -1;
    }

    /* Pass 2: Generate code */
    state->pass = 2;
    state->current_address = 0;
    state->output_size = 0;

    input = fopen(input_file, "r");
    if (!input) {
        fprintf(stderr, "Error: Cannot reopen input file '%s'\n", input_file);
        return -1;
    }

    line_num = 0;
    while (fgets(line, MAX_LINE_LEN, input)) {
        size_t len;
        line_num++;

        /* Strip newline */
        len = c32_strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        if (len > 1 && line[len-2] == '\r') {
            line[len-2] = '\0';
        }

        if (c32_asm_assemble_line(state, line, line_num) < 0) {
            fprintf(stderr, "Error: Pass 2, line %d: %s\n", line_num, line);
            state->errors++;
        }
    }

    fclose(input);

    if (state->errors > 0) {
        fprintf(stderr, "Pass 2 failed with %d errors\n", state->errors);
        return -1;
    }

    /* Write output file */
    output = fopen(output_file, "wb");
    if (!output) {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_file);
        return -1;
    }

    if (fwrite(state->output, 1, state->output_size, output) != state->output_size) {
        fprintf(stderr, "Error: Failed to write output\n");
        fclose(output);
        return -1;
    }

    fclose(output);

    return 0;
}

int main(int argc, char **argv) {
    c32_asm_state_t state;
    const char *input_file;
    const char *output_file;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.asm> <output.bin>\n", argv[0]);
        fprintf(stderr, "\nCRISP-32 Assembler - Two-pass assembler for CRISP-32 ISA\n");
        fprintf(stderr, "Converts assembly language to binary machine code.\n");
        return 1;
    }

    input_file = argv[1];
    output_file = argv[2];

    /* Initialize assembler */
    c32_asm_init(&state);

    /* Assemble the file */
    if (c32_asm_assemble_file(&state, input_file, output_file) < 0) {
        fprintf(stderr, "Assembly failed.\n");
        return 1;
    }

    printf("Assembly successful:\n");
    printf("  Input:   %s\n", input_file);
    printf("  Output:  %s\n", output_file);
    printf("  Size:    %u bytes (%u instructions)\n",
           state.output_size, state.output_size / 8);
    printf("  Symbols: %d\n", state.num_symbols);

    return 0;
}
