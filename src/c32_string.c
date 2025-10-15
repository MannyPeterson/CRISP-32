/*
 * CRISP-32 Freestanding String/Memory Functions
 * Implementation
 */

#include "c32_string.h"

void *c32_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    size_t i;

    for (i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}

void *c32_memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    size_t i;

    for (i = 0; i < n; i++) {
        p[i] = (unsigned char)c;
    }

    return s;
}

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

size_t c32_strlen(const char *s) {
    size_t len = 0;

    while (s[len] != '\0') {
        len++;
    }

    return len;
}

char *c32_strcpy(char *dest, const char *src) {
    size_t i = 0;

    while (src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';

    return dest;
}

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
