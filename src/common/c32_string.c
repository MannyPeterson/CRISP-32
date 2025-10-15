/**
 * @file c32_string.c
 * @brief Freestanding String and Memory Function Implementation
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

#include "c32_string.h"

/**
 * @addtogroup memory_ops
 * @{
 */

/**
 * @brief Copy memory block
 *
 * Copies n bytes from source to destination. The memory areas must not
 * overlap (use memmove for overlapping regions, if available).
 * This is a simple byte-by-byte copy implementation.
 *
 * @param dest Destination pointer
 * @param src Source pointer
 * @param n Number of bytes to copy
 * @return Pointer to destination
 */
void *c32_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

/**
 * @brief Fill memory with constant byte
 *
 * Fills the first n bytes of the memory area pointed to by s with
 * the constant byte c. The value c is converted to unsigned char.
 *
 * @param s Pointer to memory area
 * @param c Value to fill (converted to unsigned char)
 * @param n Number of bytes to fill
 * @return Pointer to memory area
 */
void *c32_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    size_t i;

    for (i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }

    return s;
}

/**
 * @brief Compare memory blocks
 *
 * Compares the first n bytes of the memory areas s1 and s2.
 * Returns an integer less than, equal to, or greater than zero
 * if the first n bytes of s1 is found to be less than, equal to,
 * or greater than the first n bytes of s2.
 *
 * @param s1 First memory area
 * @param s2 Second memory area
 * @param n Number of bytes to compare
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int c32_memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    size_t i;

    for (i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}

/** @} */ /* end of memory_ops */

/**
 * @addtogroup string_ops
 * @{
 */

/**
 * @brief Calculate string length
 *
 * Returns the number of bytes in the string pointed to by s,
 * excluding the terminating null byte ('\0').
 *
 * @param s Null-terminated string
 * @return Length of string in bytes (excluding null terminator)
 */
size_t c32_strlen(const char *s) {
    size_t len = 0;

    while (s[len] != '\0') {
        len++;
    }

    return len;
}

/**
 * @brief Copy string
 *
 * Copies the string pointed to by src, including the terminating
 * null byte ('\0'), to the buffer pointed to by dest. The strings
 * may not overlap, and the destination buffer must be large enough
 * to receive the copy.
 *
 * @param dest Destination buffer
 * @param src Source null-terminated string
 * @return Pointer to destination
 */
char *c32_strcpy(char *dest, const char *src) {
    size_t i = 0;

    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';

    return dest;
}

/**
 * @brief Compare strings
 *
 * Compares the two null-terminated strings s1 and s2 lexicographically.
 * Returns an integer less than, equal to, or greater than zero if s1
 * is found to be less than, equal to, or greater than s2, respectively.
 *
 * @param s1 First string
 * @param s2 Second string
 * @return 0 if equal, <0 if s1 < s2, >0 if s1 > s2
 */
int c32_strcmp(const char *s1, const char *s2) {
    size_t i = 0;

    while (s1[i] != '\0' && s2[i] != '\0') {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        i++;
    }

    return (unsigned char)s1[i] - (unsigned char)s2[i];
}

/** @} */ /* end of string_ops */
