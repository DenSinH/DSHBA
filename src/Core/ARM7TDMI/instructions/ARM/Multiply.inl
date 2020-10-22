/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 *
 * We kind of "hack in" a fake class, the INLINED_INCLUDES macro is defined right before these files are included
 * This is so that IDEs know what is going on (sort of)
 * */

#ifndef INLINED_INCLUDES
#include "../../ARM7TDMI.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

template<bool A, bool S>
void Multiply(u32 instruction) {
    u8 rd = (instruction & 0x000f'0000) >> 16;
    u32 Rn = Registers[(instruction & 0xf000) >> 12];
    u32 Rs = Registers[(instruction & 0x0f00) >> 8];
    u32 Rm = Registers[(instruction & 0x000f) >> 8];

    if constexpr(A) {
        Registers[rd] = Rm * Rs + Rn;
    }
    else {
        Registers[rd] = Rm * Rs;
    }

    if constexpr(S) {
        // C and V flags are garbage
        SetNZ(Registers[rd]);
    }

    // todo: timings
    timer++;
}

template<bool U, bool A, bool S>
void MultiplyLong(u32 instruction) {
    u8 rdhi = ((instruction & 0x000f'0000) >> 16);
    u8 rdlo = ((instruction & 0x0000'f000) >> 12);
    u32 Rs  = Registers[(instruction & 0x0000'0f00) >> 8];
    u32 Rm  = Registers[instruction & 0x0000'000f];

    if constexpr(A) {
        // accumulate
        if constexpr(!U) {
            // unsigned
            u64 rdhi_rdlo = ((u64)Registers[rdhi] << 32) | Registers[rdlo];
            u64 result    = ((u64)Rs) * ((u32)Rm) + rdhi_rdlo;
            Registers[rdhi] = (u32)(result >> 32);
            Registers[rdlo] = (u32)result;
        }
        else {
            // signed
            i64 rdhi_rdlo = ((i64)((i32)Registers[rdhi]) << 32) | Registers[rdlo];
            i64 result    = (i64)((i32)Rs) * (i64)((i32)Rm) + rdhi_rdlo;
            Registers[rdhi] = (u32)(result >> 32);
            Registers[rdlo] = (u32)result;
        }
    }
    else {
        // normal
        if constexpr(!U) {
            // unsigned
            u64 result    = ((u64)Rs) * ((u32)Rm);
            Registers[rdhi] = (u32)(result >> 32);
            Registers[rdlo] = (u32)result;
        }
        else {
            // signed
            i64 result    = (i64)((i32)Rs) * (i64)((i32)Rm);
            Registers[rdhi] = (u32)(result >> 32);
            Registers[rdlo] = (u32)result;
        }
    }

    if constexpr(S) {
        CPSR &= ~(static_cast<u32>(CPSRFlags::N) | static_cast<u32>(CPSRFlags::Z));
        CPSR |= Registers[rdhi] & 0x8000'0000;
        CPSR |= ((Registers[rdhi] == 0) && (Registers[rdlo] == 0)) ? static_cast<u32>(CPSRFlags::Z) : 0;
    }

    // todo: timings
    timer++;
}

#ifndef INLINED_INCLUDES
};
#endif