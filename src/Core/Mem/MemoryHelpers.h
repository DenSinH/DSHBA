#pragma once

#include "default.h"
#include "log.h"

#include <type_traits>

template<typename T>
static ALWAYS_INLINE T ReadArray(const u8 array[], u32 address);

template<>
u8 ReadArray(const u8 array[], u32 address) {
    return array[address];
}

template<>
u16 ReadArray(const u8 array[], u32 address) {
#ifndef DIRTY_MEMORY_ACCESS
    address &= ~1;
    return (array[address + 1] << 8) | array[address];
#else
    return *(u16*)&array[address & ~1];
#endif
}

template<>
u32 ReadArray(const u8 array[], u32 address) {
#ifndef DIRTY_MEMORY_ACCESS
    address &= ~3;
    return *(u32*)&array[address];
#else
    return *(u32*)&array[address & ~3];
#endif
}

template<typename T>
static ALWAYS_INLINE void WriteArray(u8 array[], u32 address, T value);

template<>
void WriteArray(u8 array[], u32 address, u8 value) {
    array[address] = value;
}

template<>
void WriteArray(u8 array[], u32 address, u16 value) {
#ifndef DIRTY_MEMORY_ACCESS
    address &= ~1;
    array[address]     = (value & 0xff);
    array[address + 1] = value >> 8;
#else
    *(u16*)&array[address & ~1] = value;
#endif
}

template<>
void WriteArray(u8 array[], u32 address, u32 value) {
#ifndef DIRTY_MEMORY_ACCESS
    address &= ~3;
    array[address]     =  value        & 0xff;
    array[address + 1] = (value >> 8)  & 0xff;
    array[address + 2] = (value >> 16) & 0xff;
    array[address + 3] = (value >> 24) & 0xff;
#else
    *(u32*)&array[address & ~3] = value;
#endif
}
