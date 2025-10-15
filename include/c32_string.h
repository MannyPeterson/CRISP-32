/*
 * CRISP-32 Freestanding String/Memory Functions
 * No libc dependencies
 */

#ifndef C32_STRING_H
#define C32_STRING_H

#include "c32_types.h"

/* Memory operations */
void *c32_memcpy(void *dest, const void *src, size_t n);
void *c32_memset(void *s, int c, size_t n);
int c32_memcmp(const void *s1, const void *s2, size_t n);

/* String operations */
size_t c32_strlen(const char *s);
char *c32_strcpy(char *dest, const char *src);
int c32_strcmp(const char *s1, const char *s2);

#endif /* C32_STRING_H */
