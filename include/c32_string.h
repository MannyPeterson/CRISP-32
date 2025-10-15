/**
 * @file c32_string.h
 * @brief Freestanding string and memory manipulation functions
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

#ifndef C32_STRING_H
#define C32_STRING_H

#include "c32_types.h"

/**
 * @defgroup string_functions String and Memory Functions
 * @brief Freestanding implementations of common C library functions
 *
 * These functions provide string and memory manipulation without
 * requiring libc, suitable for freestanding environments.
 * @{
 */

/**
 * @defgroup memory_ops Memory Operations
 * @brief Low-level memory manipulation
 * @{
 */

/**
 * @brief Copy memory block
 *
 * Copies n bytes from source to destination. The memory areas
 * must not overlap (use memmove for overlapping regions).
 *
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 */
void *c32_memcpy(void *dest, const void *src, size_t n);

/**
 * @brief Fill memory with constant byte
 *
 * Fills the first n bytes of memory area with the constant byte c.
 *
 * @param s Pointer to memory area
 * @param c Value to fill (converted to unsigned char)
 * @param n Number of bytes to fill
 * @return Pointer to memory area
 */
void *c32_memset(void *s, int c, size_t n);

/**
 * @brief Compare memory blocks
 *
 * Compares the first n bytes of two memory areas.
 *
 * @param s1 First memory area
 * @param s2 Second memory area
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int c32_memcmp(const void *s1, const void *s2, size_t n);

/** @} */ /* end of memory_ops */

/**
 * @defgroup string_ops String Operations
 * @brief Null-terminated string manipulation
 * @{
 */

/**
 * @brief Calculate string length
 *
 * Returns the length of the string (excluding null terminator).
 *
 * @param s Null-terminated string
 * @return Length of string in bytes
 */
size_t c32_strlen(const char *s);

/**
 * @brief Copy string
 *
 * Copies the source string (including null terminator) to destination.
 * Destination must be large enough to receive the copy.
 *
 * @param dest Destination buffer
 * @param src Source null-terminated string
 * @return Pointer to destination
 */
char *c32_strcpy(char *dest, const char *src);

/**
 * @brief Compare strings
 *
 * Compares two null-terminated strings lexicographically.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int c32_strcmp(const char *s1, const char *s2);

/** @} */ /* end of string_ops */

/** @} */ /* end of string_functions */

#endif /* C32_STRING_H */
