/*
 * CRISP-32 Type Definitions
 * Freestanding C89 - No libc dependencies
 */

#ifndef C32_TYPES_H
#define C32_TYPES_H

/* Basic integer types for freestanding C89 */
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;

typedef signed char        int8_t;
typedef signed short       int16_t;
typedef signed int         int32_t;

/* Note: C89 does not support long long
 * For 64-bit operations (e.g., MULH), use a struct or compiler extensions
 */

/* Size type */
typedef unsigned long size_t;

/* NULL definition */
#ifndef NULL
#define NULL ((void*)0)
#endif

#endif /* C32_TYPES_H */
