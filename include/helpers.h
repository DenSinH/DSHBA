#ifndef GC__HELPERS_H
#define GC__HELPERS_H

#include "default.h"
#include <stdbool.h>

static inline bool ADD_OVERFLOW64(u64 x, u64 y, u64 result) {
    return (((x ^ result) & (y ^ result)) >> 63) != 0;
}

static inline bool ADD_OVERFLOW32(u32 x, u32 y, u32 result) {
    return (((x ^ result) & (y ^ result)) >> 31) != 0;
}

static inline bool ADD_OVERFLOW16(u16 x, u16 y, u16 result) {
    return (((x ^ result) & (y ^ result)) >> 15) != 0;
}

static inline bool ADD_CARRY(u32 x, u32 y) {
    return (u32)x > ~((u32)y);
}

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

static inline u32 MASK(x, y) {
    int32_t mask_x = x ? (int32_t)0x80000000 >> (x - 1) : 0;
    int32_t mask_y = y == 31 ? 0 : 0xffffffffu >> (y + 1);
    return ((int32_t)(x - y - 1) >> 31) ^ (mask_x ^ mask_y);
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
