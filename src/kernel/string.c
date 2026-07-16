#include "string.h"

void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t *)dest;
    const uint8_t *s = (const uint8_t *)src;
    
    if (n == 0 || s == d) return dest;
    
    if ((uintptr_t)d % 16 == 0 && (uintptr_t)s % 16 == 0 && n >= 16) {
        uint64_t *d64 = (uint64_t *)d;
        const uint64_t *s64 = (const uint64_t *)s;
        size_t i = 0;
        
        for (; i + 8 <= n / 8; i += 8) {
            d64[i] = s64[i];
            d64[i + 1] = s64[i + 1];
            d64[i + 2] = s64[i + 2];
            d64[i + 3] = s64[i + 3];
            d64[i + 4] = s64[i + 4];
            d64[i + 5] = s64[i + 5];
            d64[i + 6] = s64[i + 6];
            d64[i + 7] = s64[i + 7];
        }
        for (; i + 4 <= n / 8; i += 4) {
            d64[i] = s64[i];
            d64[i + 1] = s64[i + 1];
            d64[i + 2] = s64[i + 2];
            d64[i + 3] = s64[i + 3];
        }
        for (; i + 2 <= n / 8; i += 2) {
            d64[i] = s64[i];
            d64[i + 1] = s64[i + 1];
        }
        for (; i + 1 <= n / 8; i++) {
            d64[i] = s64[i];
        }
        for (size_t k = i * 8; k < n; k++) {
            d[k] = s[k];
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    }
    
    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;
    uint8_t b = (uint8_t)c;
    
    if (n == 0) return s;
    
    if ((uintptr_t)p % 16 == 0 && n >= 16) {
        uint64_t *p64 = (uint64_t *)p;
        uint64_t pattern = ((uint64_t)b | ((uint64_t)b << 8) |
                           ((uint64_t)b << 16) | ((uint64_t)b << 24) |
                           ((uint64_t)b << 32) | ((uint64_t)b << 40) |
                           ((uint64_t)b << 48) | ((uint64_t)b << 56));
        
        uint64_t i;
        for (i = 0; i + 8 <= n / 8; i += 8) {
            p64[i] = pattern;
            p64[i + 1] = pattern;
            p64[i + 2] = pattern;
            p64[i + 3] = pattern;
            p64[i + 4] = pattern;
            p64[i + 5] = pattern;
            p64[i + 6] = pattern;
            p64[i + 7] = pattern;
        }
        for (; i + 4 <= n / 8; i += 4) {
            p64[i] = pattern;
            p64[i + 1] = pattern;
            p64[i + 2] = pattern;
            p64[i + 3] = pattern;
        }
        for (; i + 2 <= n / 8; i += 2) {
            p64[i] = pattern;
            p64[i + 1] = pattern;
        }
        for (; i + 1 <= n / 8; i++) {
            p64[i] = pattern;
        }
        for (size_t k = i * 8; k < n; k++) {
            p[k] = b;
        }
    } else {
        for (size_t i = 0; i < n; i++) {
            p[i] = b;
        }
    }
    
    return s;
}
