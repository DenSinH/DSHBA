#ifndef GC__HELPERS_H
#define GC__HELPERS_H

#include "default.h"
#include <stdbool.h>

#define ROTL32(uval, n) (((uval) << (n)) | ((uval) >> (32 - (n))))
#define ROTR32(uval, n) (((uval) >> (n)) | ((uval) << (32 - (n))))
#define EXTS32(val, len) (((i32)((val) << (32 - (len)))) >> (32 - (len)))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(val, min, max) MIN(max, MAX(val, min))

static inline uint8_t flip_byte(uint8_t b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

static inline u32 popcount(u32 x)
{
#if HAS_BUILTIN(__builtin_popcount)
    return __builtin_popcount(x);
#else
    u32 count = 0;
    for (; x != 0; x &= x - 1)
        count++;
    return count;
#endif
}

static inline u32 ctlz(u32 x)
{
#if HAS_BUILTIN(__builtin_clz)
    return x ? __builtin_clz(x) : 32;
#else
    // todo: binary search
    u8 n;
    for (n = 0; n < 32; n++) {
        if (x & (0x80000000 >> n)) {
            break;
        }
    }
    return n;
#endif
}

static inline u32 cttz(u32 x)
{
#if HAS_BUILTIN(__builtin_ctz)
    return x ? __builtin_ctz(x) : 32;
#else
    // todo: binary search
    u8 n;
    for (n = 0; n < 32; n++) {
        if (x & (1 << n)) {
            break;
        }
    }
    return n;
#endif
}

#endif //GC__HELPERS_H
