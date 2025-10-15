/**
 * @file test_suite.c
 * @brief CRISP-32 Unit Test Suite
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

/* Include generated test program headers */
#include "../../tests/unit/test_add.h"
#include "../../tests/unit/test_sub.h"
#include "../../tests/unit/test_mul.h"
#include "../../tests/unit/test_logical.h"
#include "../../tests/unit/test_shift.h"
#include "../../tests/unit/test_branch.h"
#include "../../tests/unit/test_load_store.h"

/**
 * @brief Test validation function for ADD instruction
 */
static int test_add_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 42);
    C32_ASSERT_REG_EQ(ctx, 2, 10);
    C32_ASSERT_REG_EQ(ctx, 3, 52);
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for SUB instruction
 */
static int test_sub_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 100);
    C32_ASSERT_REG_EQ(ctx, 2, 30);
    C32_ASSERT_REG_EQ(ctx, 3, 70);
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for MUL instruction
 */
static int test_mul_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 7);
    C32_ASSERT_REG_EQ(ctx, 2, 6);
    C32_ASSERT_REG_EQ(ctx, 3, 42);
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for logical operations
 */
static int test_logical_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 15);    /* 0x0F */
    C32_ASSERT_REG_EQ(ctx, 2, 51);    /* 0x33 */
    C32_ASSERT_REG_EQ(ctx, 3, 3);     /* 0x0F & 0x33 = 0x03 */
    C32_ASSERT_REG_EQ(ctx, 4, 63);    /* 0x0F | 0x33 = 0x3F */
    C32_ASSERT_REG_EQ(ctx, 5, 60);    /* 0x0F ^ 0x33 = 0x3C */
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for shift operations
 */
static int test_shift_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 8);
    C32_ASSERT_REG_EQ(ctx, 2, 32);    /* 8 << 2 */
    C32_ASSERT_REG_EQ(ctx, 3, 4);     /* 8 >> 1 */
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for branch instructions
 */
static int test_branch_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 5);
    C32_ASSERT_REG_EQ(ctx, 2, 5);
    C32_ASSERT_REG_EQ(ctx, 3, 1);     /* Branch was taken */
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test validation function for load/store instructions
 */
static int test_load_store_validation(c32_test_ctx_t *ctx) {
    C32_ASSERT_REG_EQ(ctx, 1, 0x12345678);
    C32_ASSERT_REG_EQ(ctx, 2, 0x12345678);
    C32_ASSERT_MEM_WORD_EQ(ctx, 0x2000, 0x12345678);
    C32_ASSERT_HALTED(ctx);
    return C32_TEST_PASS;
}

/**
 * @brief Test suite definition
 */
static const c32_test_case_t test_suite[] = {
    {
        "ADD and ADDI instructions",
        test_test_add,
        test_test_add_size,
        0x1000,
        100,
        test_add_validation
    },
    {
        "SUB instruction",
        test_test_sub,
        test_test_sub_size,
        0x1000,
        100,
        test_sub_validation
    },
    {
        "MUL instruction",
        test_test_mul,
        test_test_mul_size,
        0x1000,
        100,
        test_mul_validation
    },
    {
        "Logical operations (AND, OR, XOR)",
        test_test_logical,
        test_test_logical_size,
        0x1000,
        100,
        test_logical_validation
    },
    {
        "Shift operations (SLL, SRL)",
        test_test_shift,
        test_test_shift_size,
        0x1000,
        100,
        test_shift_validation
    },
    {
        "Branch instructions (BEQ)",
        test_test_branch,
        test_test_branch_size,
        0x1000,
        100,
        test_branch_validation
    },
    {
        "Load/Store instructions (LW, SW)",
        test_test_load_store,
        test_test_load_store_size,
        0x1000,
        100,
        test_load_store_validation
    }
};

/**
 * @brief Main entry point for test suite
 */
int main(void) {
    c32_test_results_t results;
    int test_count = sizeof(test_suite) / sizeof(test_suite[0]);

    /* Run all tests */
    c32_run_test_suite(test_suite, test_count, &results);

    /* Print summary */
    c32_test_print_results(&results);

    /* Return non-zero if any tests failed */
    return (results.failed > 0 || results.errors > 0) ? 1 : 0;
}
