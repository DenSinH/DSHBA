#ifndef GC__DEFAULT_H
#define GC__DEFAULT_H

#include <stdint.h>
#include <float.h>

#ifdef _MSC_VER

#define STATIC_ASSERT static_assert

#define STRCPY(dest, len, src) strcpy_s(dest, len, src)
#define SSCANF sscanf_s
#define SPRINTF sprintf_s
#define FOPEN(stream, fname, mode) fopen_s(stream, fname, mode)

#else

// holds size_t and things like that
#include <stddef.h>

#define STATIC_ASSERT _Static_assert

#define STRCPY(dest, len, src) strcpy(dest, src)
#define SSCANF sscanf
#define SPRINTF(dest, len, src, ...) sprintf(dest, src, ##__VA_ARGS__)
#define FOPEN(stream, fname, mode) *stream = fopen(fname, mode)

#endif

#define ALWAYS_INLINE inline __attribute__((always_inline))

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

#ifndef __has_builtin
#ifdef __is_identifier
#define __has_builtin(x) __is_identifier(x)
#else
#define __has_builtin(x) 0
#endif
#endif

#ifndef __is_identifier
#define __is_identifier(x) 0
#endif

#endif //GC__DEFAULT_H
