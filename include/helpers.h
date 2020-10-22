#ifndef GC__HELPERS_H
#define GC__HELPERS_H

#include "default.h"

#ifdef _MSC_VER
#include <intrin.h>     // used in a bunch of stuff here
#include <immintrin.h>
#endif

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>  // Not just <immintrin.h> for compilers other than icc
#endif

#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(__x86_64__) || defined(__i386__)

static inline __attribute__((always_inline)) u32 ROTR32(u32 uval, u32 rot) {
    return _rotr(uval, rot);  // gcc, icc, msvc.  Intel-defined.
}

static inline __attribute__((always_inline)) u32 ROTL32(u32 uval, u32 rot) {
    return _rotl(uval, rot);  // gcc, icc, msvc.  Intel-defined.
}

#else
static inline __attribute__((always_inline)) u32 ROTR32(u32 uval, u32 rot) {
    return (((uval) >> (n)) | ((uval) << (32 - (n))));
}

static inline __attribute__((always_inline)) u32 ROTL32(u32 uval, u32 rot) {
    return (((uval) << (n)) | ((uval) >> (32 - (n))));
}
#endif

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

static inline __attribute__((always_inline)) u32 popcount(u32 x)

{
#if HAS_BUILTIN(__builtin_popcount)
    return __builtin_popcount(x);
#elif defined(__x86_64__) || defined(__i386__)
    return __popcnt(x);
#else
    u32 count = 0;
    for (; x != 0; x &= x - 1)
        count++;
    return count;
#endif
}

static inline __attribute__((always_inline)) u32 ctlz(u32 x)
{
#if HAS_BUILTIN(__builtin_clz)
    return x ? __builtin_clz(x) : 32;
#elif defined(__x86_64__) || defined(__i386__)
    return __lzcnt(x);
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

static inline __attribute__((always_inline)) u32 cttz(u32 x)
{
#if HAS_BUILTIN(__builtin_ctz)
    return x ? __builtin_ctz(x) : 32;
#elif defined(__x86_64__) || defined(__i386__)
    return _tzcnt_u32(x);
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
