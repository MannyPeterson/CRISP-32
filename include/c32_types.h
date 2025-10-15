/**
 * @file c32_types.h
 * @brief Type definitions for CRISP-32 Virtual Machine
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

#ifndef C32_TYPES_H
#define C32_TYPES_H

/**
 * @defgroup types Fundamental Types
 * @brief Freestanding C89 type definitions
 *
 * These types replace stdint.h for freestanding environments.
 * No standard library dependencies.
 * @{
 */

/** @brief Unsigned 8-bit integer */
typedef unsigned char      uint8_t;

/** @brief Unsigned 16-bit integer */
typedef unsigned short     uint16_t;

/** @brief Unsigned 32-bit integer */
typedef unsigned int       uint32_t;

/** @brief Signed 8-bit integer */
typedef signed char        int8_t;

/** @brief Signed 16-bit integer */
typedef signed short       int16_t;

/** @brief Signed 32-bit integer */
typedef signed int         int32_t;

/**
 * @note 64-bit types (uint64_t, int64_t) are not available in C89.
 * C89 does not support the 'long long' type, so we cannot provide
 * uint64_t or int64_t. This limits MULH/MULHU instruction implementation.
 */

/** @brief Size type for memory operations */
typedef unsigned long size_t;

/** @brief NULL pointer constant */
#ifndef NULL
#define NULL ((void*)0)
#endif

/** @} */ /* end of types group */

#endif /* C32_TYPES_H */
