/**
 * @file c32_test.h
 * @brief CRISP-32 Unit Testing Framework
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

#ifndef C32_TEST_H
#define C32_TEST_H

#include "c32_vm.h"
#include "c32_types.h"

/**
 * @defgroup test_framework Test Framework
 * @brief JUnit-style unit testing framework for CRISP-32
 *
 * Provides a comprehensive testing framework for validating VM execution:
 * - Test case definitions with embedded programs
 * - Assertion macros for checking VM state
 * - Test runner with pass/fail reporting
 * - Test suite organization
 * @{
 */

/** @brief Maximum length for test case names */
#define C32_TEST_MAX_NAME 64

/** @brief Maximum length for assertion failure messages */
#define C32_TEST_MAX_MSG 256

/** @brief Default program load address */
#define C32_TEST_DEFAULT_LOAD_ADDR 0x1000

/** @brief Default maximum execution steps (prevents infinite loops) */
#define C32_TEST_DEFAULT_MAX_STEPS 1000

/**
 * @brief Test result status codes
 */
typedef enum {
    C32_TEST_PASS = 0,      /**< Test passed */
    C32_TEST_FAIL = 1,      /**< Assertion failed */
    C32_TEST_ERROR = 2      /**< Runtime error (VM crash, timeout, etc.) */
} c32_test_status_t;

/**
 * @brief Test context passed to test functions
 *
 * Contains VM state and utilities for assertions
 */
typedef struct {
    c32_vm_t *vm;                           /**< Pointer to VM instance */
    char failure_msg[C32_TEST_MAX_MSG];     /**< Buffer for failure messages */
    int has_failure;                        /**< Flag indicating test failure */
} c32_test_ctx_t;

/**
 * @brief Test function signature
 *
 * Test functions receive a context with VM state and can use
 * assertion macros to validate expected behavior.
 *
 * @param ctx Test context containing VM state
 * @return C32_TEST_PASS on success, C32_TEST_FAIL or C32_TEST_ERROR on failure
 */
typedef int (*c32_test_fn_t)(c32_test_ctx_t *ctx);

/**
 * @brief Test case definition
 *
 * Defines a single test case including the program to execute
 * and the validation function to check results.
 */
typedef struct {
    const char *name;           /**< Test case name */
    const uint8_t *program;     /**< Pointer to program binary */
    uint32_t program_size;      /**< Size of program in bytes */
    uint32_t load_addr;         /**< Address to load program (default: 0x1000) */
    uint32_t max_steps;         /**< Maximum execution steps (default: 1000) */
    c32_test_fn_t test_fn;      /**< Test validation function */
} c32_test_case_t;

/**
 * @brief Test suite results
 *
 * Accumulates pass/fail statistics for a test suite
 */
typedef struct {
    int total;      /**< Total number of tests */
    int passed;     /**< Number of tests passed */
    int failed;     /**< Number of tests failed */
    int errors;     /**< Number of tests with errors */
} c32_test_results_t;

/* ========================================================================
 * Assertion Macros
 * ======================================================================== */

/**
 * @brief Assert that a register equals an expected value
 *
 * @param ctx Test context
 * @param reg Register number (0-31)
 * @param expected Expected register value
 */
#define C32_ASSERT_REG_EQ(ctx, reg, expected) \
    do { \
        uint32_t _actual = (ctx)->vm->regs[(reg)]; \
        if (_actual != (expected)) { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "Register R%d: expected 0x%08lx, got 0x%08lx", \
                (reg), (unsigned long)(expected), (unsigned long)_actual); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Assert that the program counter equals an expected value
 *
 * @param ctx Test context
 * @param expected Expected PC value
 */
#define C32_ASSERT_PC_EQ(ctx, expected) \
    do { \
        uint32_t _actual = (ctx)->vm->pc; \
        if (_actual != (expected)) { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "PC: expected 0x%08lx, got 0x%08lx", \
                (unsigned long)(expected), (unsigned long)_actual); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Assert that a memory byte equals an expected value
 *
 * @param ctx Test context
 * @param addr Memory address
 * @param expected Expected byte value
 */
#define C32_ASSERT_MEM_BYTE_EQ(ctx, addr, expected) \
    do { \
        if ((addr) < (ctx)->vm->memory_size) { \
            uint8_t _actual = (ctx)->vm->memory[(addr)]; \
            if (_actual != (expected)) { \
                c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                    "Memory[0x%08lx]: expected 0x%02x, got 0x%02x", \
                    (unsigned long)(addr), (unsigned int)(expected), (unsigned int)_actual); \
                (ctx)->has_failure = 1; \
                return C32_TEST_FAIL; \
            } \
        } else { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "Memory[0x%08lx]: address out of bounds", (unsigned long)(addr)); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Assert that a memory word equals an expected value
 *
 * @param ctx Test context
 * @param addr Memory address (must be 4-byte aligned)
 * @param expected Expected word value
 */
#define C32_ASSERT_MEM_WORD_EQ(ctx, addr, expected) \
    do { \
        if ((addr) + 4 <= (ctx)->vm->memory_size) { \
            uint32_t _actual = c32_read_word((ctx)->vm->memory + (addr)); \
            if (_actual != (expected)) { \
                c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                    "Memory[0x%08lx]: expected 0x%08lx, got 0x%08lx", \
                    (unsigned long)(addr), (unsigned long)(expected), (unsigned long)_actual); \
                (ctx)->has_failure = 1; \
                return C32_TEST_FAIL; \
            } \
        } else { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "Memory[0x%08lx]: address out of bounds", (unsigned long)(addr)); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Assert that the VM is no longer running (halted)
 *
 * @param ctx Test context
 */
#define C32_ASSERT_HALTED(ctx) \
    do { \
        if ((ctx)->vm->running) { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "VM still running: expected halted state"); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Assert that the VM is still running
 *
 * @param ctx Test context
 */
#define C32_ASSERT_RUNNING(ctx) \
    do { \
        if (!(ctx)->vm->running) { \
            c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, \
                "VM halted: expected running state"); \
            (ctx)->has_failure = 1; \
            return C32_TEST_FAIL; \
        } \
    } while (0)

/**
 * @brief Fail test with custom message
 *
 * @param ctx Test context
 * @param msg Failure message
 */
#define C32_ASSERT_FAIL(ctx, msg) \
    do { \
        c32_test_snprintf((ctx)->failure_msg, C32_TEST_MAX_MSG, "%s", (msg)); \
        (ctx)->has_failure = 1; \
        return C32_TEST_FAIL; \
    } while (0)

/* ========================================================================
 * Test Framework Functions
 * ======================================================================== */

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
                                uint32_t memory_size);

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
                        c32_test_results_t *results);

/**
 * @brief Initialize test results structure
 *
 * @param results Pointer to results structure
 */
void c32_test_results_init(c32_test_results_t *results);

/**
 * @brief Print test results summary
 *
 * Outputs a summary of test results to stdout (requires hosted environment).
 *
 * @param results Pointer to results structure
 */
void c32_test_print_results(const c32_test_results_t *results);

/**
 * @brief Simplified snprintf for test messages
 *
 * C89-compatible string formatting for test messages.
 * Supports limited format specifiers: %d, %x, %08lx, %s
 *
 * @param buf Output buffer
 * @param size Buffer size
 * @param fmt Format string
 * @param ... Format arguments
 */
void c32_test_snprintf(char *buf, int size, const char *fmt, ...);

/** @} */ /* end of test_framework */

#endif /* C32_TEST_H */
