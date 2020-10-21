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

template<bool I, bool P, bool U, bool B, bool W, bool L, u8 shift_type>
void SingleDataTransfer(u32 instruction) {
    u8 Rn = ((instruction & 0x000f'0000) >> 16);
    u8 Rd = ((instruction & 0x0000'f000) >> 12);

    u32 address = Registers[Rn];
    u32 offset;
    if constexpr(!I) {
        // immediate offset
        offset = instruction & 0x0fff;
    }
    else {
        // register specified offset
        // same as for data processing, except no register specified shift amounts
        offset = GetShiftedRegister<false, shift_type, true>(instruction);
    }

    if constexpr(P) {
        // pre-index
        if constexpr(U) {
            // up
            address += offset;
        }
        else {
            // down
            address -= offset;
        }
    }

    if constexpr(L) {
        log_cpu_verbose("LDR%c r%d, [r%d, #%x]%c", B ? 'B' : ' ', Rd, Rn, offset, W ? '!' : ' ');
        if constexpr(B) {
            Registers[Rd] = Memory->Mem::Read<u8, true>(address);
        }
        else {
            u32 loaded = Memory->Mem::Read<u32, true>(address);
            // misaligned loads fetch rotated value
            u32 rot = (address & 3) << 3;
            if (rot != 0) [[unlikely]] {
                loaded = ROTR32(loaded, rot);
            }

            Registers[Rd] = loaded;

            if (Rd == 15) {
                FlushPipeline();
            }
        }
    }
    else {
        log_cpu_verbose("STR%c r%d, [r%d, #%x]%c", B ? 'B' : ' ', Rd, Rn, offset, W ? '!' : ' ');
        u32 value = Registers[Rd];

        if (Rd == 15) [[unlikely]] {
            // correction for pipeline effects
            value += 4;
        }

        if constexpr(B) {
            Memory->Mem::Write<u8, true>(address, (u8)value);
        }
        else {
            // stores are force aligned
            Memory->Mem::Write<u32, true>(address, value);
        }
    }

    if constexpr(W || !P) {
        if (!(L && Rn == Rd)) {
            if constexpr(!P) {
                // post-index
                if constexpr(U) {
                    address += offset;
                }
                else {
                    address -= offset;
                }
            }

            // writeback register will not be PC, I don't know what would happen in that case
            Registers[Rn] = address;
        }
    }

    if constexpr(L) {
        // internal cycle
        timer++;
    }
}


template<bool P, bool U, bool imm_offset, bool W, bool L, bool S, bool H>
void HalfwordDataTransfer(u32 instruction) {
    u32 offset;
    u8 Rn = (instruction & 0x000f'0000) >> 16;
    u8 Rd = (instruction & 0xf000) >> 12;
    u32 address = Registers[Rn];

    if constexpr(imm_offset) {
        offset = ((instruction & 0x0f00) >> 4) | (instruction & 0x000f);
    }
    else {
        offset = Registers[instruction & 0xf];
    }

    if constexpr(P) {
        // Pre-indexed
        if constexpr(U) {
            // Up
            address += offset;
        }
        else {
            address -= offset;
        }
    }

    if constexpr(S) {
        if constexpr(H) {
            // Signed byte
            if constexpr(L) {
                Registers[Rd] = (u32)((i32)((i8)Memory->Mem::Read<u8, true>(address)));
                log_cpu_verbose("LDRSB r%d, [r%d, #%x]%c", Rd, Rn, offset, W ? '!' : '\0');
            }
            else {
                log_fatal("Cannot store signed byte");
            }
        }
        else {
            // signed halfword
            if constexpr(L) {
                log_cpu_verbose("LDRSH r%d, [r%d, #%x]%c", Rd, Rn, offset, W ? '!' : '\0');
                if (address & 1) {
                    // misaligned, loads sign extended byte instead
                    Registers[Rd] = (u32)((i32)((i8)Memory->Mem::Read<u8, true>(address)));
                }
                else {
                    Registers[Rd] = (u32)((i32)((i16)Memory->Mem::Read<u16, true>(address)));
                }
            }
            else {
                log_fatal("Cannot store signed halfword");
            }
        }
    }
    else {
        if constexpr(H) {
            // unsigned halfwords
            if constexpr(L) {
                log_cpu_verbose("LDRH r%d, [r%d, #%x]%c", Rd, Rn, offset, W ? '!' : '\0');
                if (address & 1) {
                    // misaligned
                    u32 loaded = Memory->Mem::Read<u16, true>(address);
                    Registers[Rd] = ROTR32(loaded, 8);
                }
                else {
                    Registers[Rd] = Memory->Mem::Read<u16, true>(address);
                }
            }
            else {
                log_cpu_verbose("STRH r%d, [r%d, #%x]%c", Rd, Rn, offset, W ? '!' : '\0');
                Memory->Mem::Write<u16, true>(address, (u16)Registers[Rd]);
            }
        }
        else {
            log_fatal("Unintended SWP instruction");
        }
    }

    if constexpr((W || !P)) {
        // writeback (post indexing is always written back)
        if (!(Rn == Rd && L)) {
            if constexpr(!P) {
                // Post-indexed
                if constexpr(U) {
                    // Up
                    address += offset;
                }
                else {
                    address -= offset;
                }
            }

            Registers[Rn] = address;
        }
    }

    if constexpr(L) {
        // internal cycle
        timer++;
    }
}

#ifndef INLINED_INCLUDES
};
#endif