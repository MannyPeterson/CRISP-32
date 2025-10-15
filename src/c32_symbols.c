/*
 * CRISP-32 Symbol Table Management
 */

#include "c32_asm.h"
#include "c32_string.h"

/* Initialize assembler state */
void c32_asm_init(c32_asm_state_t *state) {
    int i;

    state->num_symbols = 0;
    state->current_address = 0;
    state->output_size = 0;
    state->pass = 1;
    state->errors = 0;

    /* Clear symbol table */
    for (i = 0; i < MAX_SYMBOLS; i++) {
        state->symbols[i].name[0] = '\0';
        state->symbols[i].address = 0;
        state->symbols[i].defined = 0;
    }

    /* Clear output buffer */
    c32_memset(state->output, 0, MAX_OUTPUT_SIZE);
}

/* Add a symbol to the symbol table */
int c32_asm_add_symbol(c32_asm_state_t *state, const char *name, uint32_t address) {
    int idx;
    size_t name_len;

    if (!state || !name) {
        return -1;
    }

    name_len = c32_strlen(name);
    if (name_len == 0 || name_len >= MAX_LABEL_LEN) {
        return -1;
    }

    /* Check if symbol already exists */
    idx = c32_asm_find_symbol(state, name);
    if (idx >= 0) {
        /* Symbol exists - update it if not yet defined */
        if (state->symbols[idx].defined) {
            /* Duplicate definition */
            return -2;
        }
        state->symbols[idx].address = address;
        state->symbols[idx].defined = 1;
        return idx;
    }

    /* Add new symbol */
    if (state->num_symbols >= MAX_SYMBOLS) {
        return -3; /* Symbol table full */
    }

    idx = state->num_symbols;
    c32_strcpy(state->symbols[idx].name, name);
    state->symbols[idx].address = address;
    state->symbols[idx].defined = 1;
    state->num_symbols++;

    return idx;
}

/* Find a symbol in the symbol table */
int c32_asm_find_symbol(c32_asm_state_t *state, const char *name) {
    int i;

    if (!state || !name) {
        return -1;
    }

    for (i = 0; i < state->num_symbols; i++) {
        if (c32_strcmp(state->symbols[i].name, name) == 0) {
            return i;
        }
    }

    return -1;
}
