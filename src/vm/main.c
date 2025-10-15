/*
 * CRISP-32 Virtual Machine - Main Entry Point
 * Example usage and test harness
 */

#include "c32_vm.h"
#include "c32_string.h"

/* Include embedded test programs */
#include "../tests/simple.h"
#include "../tests/hello.h"

/* Test program memory (64KB) */
static uint8_t test_memory[65536];

/* Simple instruction disassembler for debugging */
static void dump_instruction(uint32_t addr, const uint8_t *inst) {
    uint8_t opcode = inst[0];
    uint8_t rs = inst[1];
    uint8_t rt = inst[2];
    uint8_t rd = inst[3];
    uint32_t imm = c32_read_word(inst + 4);

    /* Print address and raw bytes */
    {
        int i;
        for (i = 0; i < 8; i++) {
            /* Placeholder to use variables */
            (void)opcode;
            (void)rs;
            (void)rt;
            (void)rd;
            (void)imm;
            (void)addr;
        }
    }

    /* TODO: Decode and print instruction mnemonic */
}

/* Load a test program into VM memory */
static void load_program(c32_vm_t *vm, const uint8_t *program, uint32_t size, uint32_t load_addr) {
    if (load_addr + size <= vm->memory_size) {
        c32_memcpy(vm->memory + load_addr, program, size);
    }
}

/* Run embedded tests */
static int run_tests(void) {
    c32_vm_t vm;
    uint32_t i;

    /* Test 1: Load simple program */
    c32_vm_init(&vm, test_memory, sizeof(test_memory));
    load_program(&vm, test_simple, test_simple_size, 0x1000);

    /* Verify first instruction loaded correctly */
    if (vm.memory[0x1000] != 0x05) {
        return -1;  /* Failed */
    }

    /* Test 2: Load hello program */
    c32_vm_init(&vm, test_memory, sizeof(test_memory));
    load_program(&vm, test_hello, test_hello_size, 0x1000);

    /* Verify program loaded */
    if (vm.memory[0x1000] != 0x05) {
        return -1;  /* Failed */
    }

    /* Dump first few instructions for verification */
    for (i = 0; i < 3 && i * 8 < test_hello_size; i++) {
        dump_instruction(0x1000 + i * 8, &vm.memory[0x1000 + i * 8]);
    }

    return 0;  /* Success */
}

int main(void) {
    int result;

    /* Run embedded tests */
    result = run_tests();

    /* For now, just run tests and exit */
    /* TODO: Implement instruction execution */
    /* TODO: Implement proper test reporting */

    return result;
}
