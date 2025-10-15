/*
 * CRISP-32 Virtual Machine - Main Entry Point
 * Example usage and test harness
 */

#include "c32_vm.h"

/* Simple test program memory (64KB) */
static uint8_t test_memory[65536];

int main(void) {
    c32_vm_t vm;

    /* Initialize VM with test memory */
    c32_vm_init(&vm, test_memory, sizeof(test_memory));

    /* TODO: Load program into memory */
    /* TODO: Set up interrupt handlers */
    /* TODO: Set PC to entry point */
    /* TODO: Run VM */

    /* For now, just initialize and exit */
    (void)vm;

    return 0;
}
