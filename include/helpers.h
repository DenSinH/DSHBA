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

static ALWAYS_INLINE u32 ROTR32(u32 uval, u32 rot) {
    return _rotr(uval, rot);  // gcc, icc, msvc.  Intel-defined.
}

static ALWAYS_INLINE u32 ROTL32(u32 uval, u32 rot) {
    return _rotl(uval, rot);  // gcc, icc, msvc.  Intel-defined.
}

#else
static ALWAYS_INLINE u32 ROTR32(u32 uval, u32 rot) {
    return (((uval) >> (n)) | ((uval) << (32 - (n))));
}

static ALWAYS_INLINE u32 ROTL32(u32 uval, u32 rot) {
    return (((uval) << (n)) | ((uval) >> (32 - (n))));
}
#endif

#define EXTS32(val, len) (((i32)((val) << (32 - (len)))) >> (32 - (len)))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define CLAMP(val, min, max) MIN(max, MAX(val, min))

static ALWAYS_INLINE uint8_t flip_byte(uint8_t b) {
#if __has_builtin(__builtin_bitreverse8)
    return __builtin_bitreverse8(b);
#else
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
#endif
}

static ALWAYS_INLINE u32 popcount(u32 x)

{
#if __has_builtin(__builtin_popcount) || defined(__GNUC__)
#define HAS_POPCOUNT
    return __builtin_popcount(x);
#elif defined(__x86_64__) || defined(__i386__)
#define HAS_POPCOUNT
    return __popcnt(x);
#else
    u32 count = 0;
    for (; x != 0; x &= x - 1)
        count++;
    return count;
#endif
}

static ALWAYS_INLINE u32 ctlz(u32 x)
{
#if __has_builtin(__builtin_clz) || defined(__GNUC__)
#define HAS_CTLZ
    return x ? __builtin_clz(x) : 32;
#elif defined(__x86_64__) || defined(__i386__)
#define HAS_CTLZ
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

static ALWAYS_INLINE u32 cttz(u32 x)
{
#if defined(__x86_64__) || defined(__i386__)
#define HAS_CTTZ
    return _tzcnt_u32(x);  // one less check (argument is defined for a 0 argument)
#elif __has_builtin(__builtin_ctz) || defined(__GNUC__)
#define HAS_CTTZ
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

#if defined(__GNUC__) || __is_identifier(__builtin_expect) || __has_builtin(__builtin_expect)
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#if __has_builtin(__builtin_unreachable)
#define UNREACHABLE __builtin_unreachable();
#else
#define UNREACHABLE
#endif

#if __has_builtin(__builtin_assume)
#define ASSUME(x) __builtin_assume(x);
#else
#define ASSUME(x)
#endif

#if __has_builtin(__builtin_assume_aligned)
#define ASSUME_ALIGNED(ptr, alignment) __builtin_assume_aligned(ptr, alignment)
#else
#define ASSUME_ALIGNED(ptr, alignment)
#endif

//#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
//    defined(__BIG_ENDIAN__) || \
//    defined(__ARMEB__) || \
//    defined(__THUMBEB__) || \
//    defined(__AARCH64EB__) || \
//    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
//#define BIG_ENDIAN
//#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
//    defined(__LITTLE_ENDIAN__) || \
//    defined(__ARMEL__) || \
//    defined(__THUMBEL__) || \
//    defined(__AARCH64EL__) || \
//    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
//#define LITTLE_ENDIAN
//#else
//#error "I don't know what architecture this is!"
//#endif

#endif //GC__HELPERS_H
