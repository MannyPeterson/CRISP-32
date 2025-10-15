/**
 * @file main.c
 * @brief CRISP-32 Virtual Machine - Standalone Binary Loader
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

#include "c32_vm.h"
#include "c32_string.h"

#include <stdio.h>
#include <stdlib.h>

/**
 * @defgroup vm_runner VM Runner
 * @brief Standalone VM that loads and executes CRISP-32 binary programs
 * @{
 */

/** @brief VM memory size (64KB) */
#define VM_MEMORY_SIZE 65536

/** @brief Default program load address */
#define DEFAULT_LOAD_ADDR 0x1000

/** @brief Maximum execution steps before timeout */
#define MAX_EXECUTION_STEPS 1000000

/** @brief VM memory buffer */
static uint8_t vm_memory[VM_MEMORY_SIZE];

/**
 * @brief Load a binary file into VM memory
 *
 * @param vm Pointer to VM instance
 * @param filename Path to binary file
 * @param load_addr Address to load program
 * @return 0 on success, -1 on failure
 */
static int load_binary_file(c32_vm_t *vm, const char *filename, uint32_t load_addr) {
    FILE *fp;
    size_t bytes_read;
    size_t available_space;

    fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return -1;
    }

    /* Calculate available memory space */
    if (load_addr >= vm->memory_size) {
        fprintf(stderr, "Error: Load address 0x%08x exceeds memory size\n",
                (unsigned int)load_addr);
        fclose(fp);
        return -1;
    }

    available_space = vm->memory_size - load_addr;

    /* Read binary into memory */
    bytes_read = fread(vm->memory + load_addr, 1, available_space, fp);

    if (ferror(fp)) {
        fprintf(stderr, "Error: Failed to read file '%s'\n", filename);
        fclose(fp);
        return -1;
    }

    fclose(fp);

    printf("Loaded %lu bytes from '%s' at address 0x%08x\n",
           (unsigned long)bytes_read, filename, (unsigned int)load_addr);

    return 0;
}

/**
 * @brief Print VM register state
 *
 * @param vm Pointer to VM instance
 */
static void print_registers(const c32_vm_t *vm) {
    int i;

    printf("\nRegister State:\n");
    printf("================\n");

    for (i = 0; i < 32; i++) {
        printf("R%-2d: 0x%08lx", i, (unsigned long)vm->regs[i]);
        if (i % 4 == 3) {
            printf("\n");
        } else {
            printf("  ");
        }
    }

    printf("PC:  0x%08lx\n", (unsigned long)vm->pc);
    printf("================\n");
}

/**
 * @brief Print usage information
 *
 * @param program_name Name of the program
 */
static void print_usage(const char *program_name) {
    printf("CRISP-32 Virtual Machine\n");
    printf("Usage: %s <binary_file> [load_address]\n\n", program_name);
    printf("Arguments:\n");
    printf("  binary_file    Path to CRISP-32 binary program\n");
    printf("  load_address   Memory address to load program (hex, default: 0x1000)\n\n");
    printf("Examples:\n");
    printf("  %s program.bin\n", program_name);
    printf("  %s program.bin 0x2000\n", program_name);
}

/**
 * @brief Main entry point
 *
 * Loads and executes a CRISP-32 binary program.
 *
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0 = success, 1 = failure)
 */
int main(int argc, char **argv) {
    c32_vm_t vm;
    uint32_t load_addr = DEFAULT_LOAD_ADDR;
    int step_count = 0;
    const char *filename;

    /* Parse arguments */
    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return 1;
    }

    filename = argv[1];

    /* Parse optional load address */
    if (argc == 3) {
        unsigned long addr;
        if (sscanf(argv[2], "0x%lx", &addr) == 1 ||
            sscanf(argv[2], "%lx", &addr) == 1) {
            load_addr = (uint32_t)addr;
        } else {
            fprintf(stderr, "Error: Invalid load address '%s'\n", argv[2]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Initialize VM */
    c32_vm_init(&vm, vm_memory, VM_MEMORY_SIZE);

    /* Load binary program */
    if (load_binary_file(&vm, filename, load_addr) != 0) {
        return 1;
    }

    /* Set PC to start of program */
    vm.pc = load_addr;
    vm.running = 1;

    printf("\nStarting execution at 0x%08x...\n", (unsigned int)load_addr);

    /* Execute program */
    while (vm.running && step_count < MAX_EXECUTION_STEPS) {
        if (c32_vm_step(&vm) != 0) {
            fprintf(stderr, "\nError: VM execution failed at PC=0x%08x\n",
                    (unsigned int)vm.pc);
            print_registers(&vm);
            return 1;
        }
        step_count++;
    }

    /* Check if program timed out */
    if (vm.running) {
        fprintf(stderr, "\nWarning: Program did not halt within %d steps\n",
                MAX_EXECUTION_STEPS);
    } else {
        printf("\nProgram halted after %d steps\n", step_count);
    }

    /* Print final register state */
    print_registers(&vm);

    return 0;
}

/** @} */ /* end of vm_runner */
