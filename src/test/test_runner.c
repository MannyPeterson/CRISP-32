/**
 * @file test_runner.c
 * @brief CRISP-32 Unit Testing Framework Implementation
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

#include "c32_test.h"
#include "c32_vm.h"
#include "c32_string.h"

#include <stdio.h>
#include <stdarg.h>

/**
 * @brief Load a test program into VM memory
 *
 * @param vm Pointer to VM instance
 * @param program Pointer to program binary
 * @param size Size of program in bytes
 * @param load_addr Address to load program
 * @return 0 on success, -1 on failure
 */
static int load_test_program(c32_vm_t *vm, const uint8_t *program,
                             uint32_t size, uint32_t load_addr) {
    if (load_addr + size > vm->memory_size) {
        return -1;
    }
    c32_memcpy(vm->memory + load_addr, program, size);
    return 0;
}

/**
 * @brief Run a single test case
 *
 * Loads the program, executes it, and runs the test validation function.
 *
 * @param test_case Pointer to test case definition
 * @param memory Memory buffer for VM (must be at least 64KB)
 * @param memory_size Size of memory buffer
 * @return Test status (PASS, FAIL, or ERROR)
 */
c32_test_status_t c32_run_test(const c32_test_case_t *test_case,
                                uint8_t *memory,
                                uint32_t memory_size) {
    c32_vm_t vm;
    c32_test_ctx_t ctx;
    uint32_t load_addr;
    uint32_t max_steps;
    int step;
    int result;

    /* Validate inputs */
    if (!test_case || !test_case->program || !test_case->test_fn || !memory) {
        return C32_TEST_ERROR;
    }

    /* Initialize VM */
    c32_vm_init(&vm, memory, memory_size);

    /* Use defaults if not specified */
    load_addr = test_case->load_addr ? test_case->load_addr : C32_TEST_DEFAULT_LOAD_ADDR;
    max_steps = test_case->max_steps ? test_case->max_steps : C32_TEST_DEFAULT_MAX_STEPS;

    /* Load program */
    if (load_test_program(&vm, test_case->program, test_case->program_size, load_addr) != 0) {
        return C32_TEST_ERROR;
    }

    /* Set PC to start of program */
    vm.pc = load_addr;
    vm.running = 1;

    /* Execute program */
    for (step = 0; step < (int)max_steps && vm.running; step++) {
        if (c32_vm_step(&vm) != 0) {
            /* VM error occurred */
            return C32_TEST_ERROR;
        }
    }

    /* Check for timeout */
    if (vm.running) {
        /* Program did not halt within max_steps */
        return C32_TEST_ERROR;
    }

    /* Initialize test context */
    ctx.vm = &vm;
    ctx.has_failure = 0;
    {
        int i;
        for (i = 0; i < C32_TEST_MAX_MSG; i++) {
            ctx.failure_msg[i] = '\0';
        }
    }

    /* Run test validation function */
    result = test_case->test_fn(&ctx);

    /* Print failure message if test failed */
    if (result == C32_TEST_FAIL && ctx.failure_msg[0] != '\0') {
        printf("    %s\n", ctx.failure_msg);
    }

    return (c32_test_status_t)result;
}

/**
 * @brief Run a suite of test cases
 *
 * Executes multiple test cases and accumulates results.
 *
 * @param tests Array of test cases
 * @param test_count Number of test cases in array
 * @param results Pointer to results structure to populate
 */
void c32_run_test_suite(const c32_test_case_t *tests,
                        int test_count,
                        c32_test_results_t *results) {
    static uint8_t test_memory[65536];
    int i;

    c32_test_results_init(results);

    printf("Running test suite (%d tests)...\n\n", test_count);

    for (i = 0; i < test_count; i++) {
        c32_test_status_t status;
        const c32_test_case_t *test = &tests[i];

        printf("[%d/%d] %s ... ", i + 1, test_count, test->name);
        fflush(stdout);

        status = c32_run_test(test, test_memory, sizeof(test_memory));

        results->total++;

        switch (status) {
            case C32_TEST_PASS:
                printf("PASS\n");
                results->passed++;
                break;
            case C32_TEST_FAIL:
                printf("FAIL\n");
                results->failed++;
                break;
            case C32_TEST_ERROR:
                printf("ERROR\n");
                results->errors++;
                break;
        }
    }

    printf("\n");
}

/**
 * @brief Initialize test results structure
 *
 * @param results Pointer to results structure
 */
void c32_test_results_init(c32_test_results_t *results) {
    results->total = 0;
    results->passed = 0;
    results->failed = 0;
    results->errors = 0;
}

/**
 * @brief Print test results summary
 *
 * Outputs a summary of test results to stdout (requires hosted environment).
 *
 * @param results Pointer to results structure
 */
void c32_test_print_results(const c32_test_results_t *results) {
    printf("========================================\n");
    printf("Test Results Summary\n");
    printf("========================================\n");
    printf("Total:  %d\n", results->total);
    printf("Passed: %d\n", results->passed);
    printf("Failed: %d\n", results->failed);
    printf("Errors: %d\n", results->errors);
    printf("========================================\n");

    if (results->failed == 0 && results->errors == 0) {
        printf("All tests passed!\n");
    } else {
        printf("Some tests failed.\n");
    }
}

/**
 * @brief Simplified snprintf for test messages
 *
 * C89-compatible string formatting for test messages.
 * Supports limited format specifiers: %d, %x, %08lx, %02x, %s
 *
 * @param buf Output buffer
 * @param size Buffer size
 * @param fmt Format string
 * @param ... Format arguments
 */
void c32_test_snprintf(char *buf, int size, const char *fmt, ...) {
    va_list args;

    /* Use standard vsnprintf for simplicity (C99 but widely supported) */
    /* For strict C89, would need manual implementation */
    va_start(args, fmt);

    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        /* C99 or later */
        vsnprintf(buf, (size_t)size, fmt, args);
    #else
        /* C89 fallback - use vsprintf with manual size checking */
        /* Note: This is not fully safe but works for our use case */
        vsprintf(buf, fmt, args);
        (void)size; /* Suppress unused parameter warning */
    #endif

    va_end(args);
}
