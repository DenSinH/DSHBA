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
void __fastcall SingleDataTransfer(u32 instruction) {
    u8 rn = ((instruction & 0x000f'0000) >> 16);
    u8 rd = ((instruction & 0x0000'f000) >> 12);

    u32 address = Registers[rn];
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
        log_cpu_verbose("LDR%c r%d, [r%d, #%x]%c", B ? 'B' : ' ', rd, rn, offset, W ? '!' : ' ');
        if constexpr(B) {
            Registers[rd] = Memory->Mem::Read<u8, true>(address);
        }
        else {
            u32 loaded = Memory->Mem::Read<u32, true>(address);
            // misaligned loads fetch rotated value
            u32 rot = (address & 3) << 3;
            if (unlikely(rot != 0)) {
                loaded = ROTR32(loaded, rot);
            }

            Registers[rd] = loaded;
        }

        if (unlikely(rd == 15)) {
            Memory->OpenBusOverride   = pc;  // loaded value
            FakePipelineFlush<true>();
            Memory->OpenBusOverrideAt = pc + 4;  // instruction + 8 of where the load happened
        }
    }
    else {
        log_cpu_verbose("STR%c r%d, [r%d, #%x]%c", B ? 'B' : ' ', rd, rn, offset, W ? '!' : ' ');
        u32 value = Registers[rd];

        if (unlikely(rd == 15)) {
            // correction for pipeline effects
            value += 4;
        }

        if constexpr(B) {
            Memory->Mem::Write<u8, true, true>(address, (u8)value);
        }
        else {
            // stores are force aligned
            Memory->Mem::Write<u32, true, true>(address, value);
        }
    }

    if constexpr(W || !P) {
        if (!(L && rn == rd)) {
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
            Registers[rn] = address;
        }
    }

    if constexpr(L) {
        // internal cycle
        (*timer)++;
    }
}


template<bool P, bool U, bool imm_offset, bool W, bool L, bool S, bool H>
void __fastcall HalfwordDataTransfer(u32 instruction) {
    u32 offset;
    u8 rn = (instruction & 0x000f'0000) >> 16;
    u8 rd = (instruction & 0xf000) >> 12;
    u32 address = Registers[rn];

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
            // signed halfword
            if constexpr(L) {
                log_cpu_verbose("LDRSH r%d, [r%d, #%x]%c", rd, rn, offset, W ? '!' : '\0');
                if (unlikely(address & 1)) {
                    // misaligned, loads sign extended byte instead (we load a halfword to fix the BIOS readstate)
                    Registers[rd] = (u32)((i32)((i8)(Memory->Mem::Read<u16, true>(address) >> 8)));
                }
                else {
                    Registers[rd] = (u32)((i32)((i16)Memory->Mem::Read<u16, true>(address)));
                }
            }
            else {
                log_fatal("Cannot store signed halfword");
            }
        }
        else {
            // Signed byte
            if constexpr(L) {
                Registers[rd] = (u32)((i32)((i8)Memory->Mem::Read<u8, true>(address)));
                log_cpu_verbose("LDRSB r%d, [r%d, #%x]%c", rd, rn, offset, W ? '!' : '\0');
            }
            else {
                log_fatal("Cannot store signed byte");
            }
        }
    }
    else {
        if constexpr(H) {
            // unsigned halfwords
            if constexpr(L) {
                log_cpu_verbose("LDRH r%d, [r%d, #%x]%c", rd, rn, offset, W ? '!' : '\0');
                if (unlikely(address & 1)) {
                    // misaligned
                    u32 loaded = Memory->Mem::Read<u16, true>(address);
                    Registers[rd] = ROTR32(loaded, 8);
                }
                else {
                    Registers[rd] = Memory->Mem::Read<u16, true>(address);
                }
            }
            else {
                log_cpu_verbose("STRH r%d, [r%d, #%x]%c", rd, rn, offset, W ? '!' : '\0');
                Memory->Mem::Write<u16, true, true>(address, (u16)Registers[rd]);
            }
        }
        else {
            UNREACHABLE
            log_fatal("Unintended SWP instruction");
        }
    }

    if constexpr((W || !P)) {
        // writeback (post indexing is always written back)
        if (!((rn == rd) && L)) {
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

            Registers[rn] = address;
        }
    }

    if constexpr(L) {
        // internal cycle
        (*timer)++;
    }
}

template<bool B>
void __fastcall SWP(u32 instruction) {
    u32 address = Registers[(instruction & 0x000f'0000) >> 16];
    u32 rd = (instruction & 0xf000) >> 12;
    u32 rm = (instruction & 0x000f);

    if constexpr(B) {
        // can not directly load because rd and rm might be the same
        u8 memory_content = Memory->Mem::Read<u8, true>(address);
        Memory->Mem::Write<u8, true, true>(address, Registers[rm]);
        Registers[rd] = memory_content;
    }
    else {
        // can not directly load because rd and rm might be the same
        u32 memory_content = Memory->Mem::Read<u32, true>(address);
        Memory->Mem::Write<u32, true, true>(address, Registers[rm]);

        // misaligned reads
        u8 rotate_amount = (address & 3) << 3;
        if (unlikely(rotate_amount != 0)) {
            memory_content = ROTR32(memory_content, rotate_amount);
        }
        Registers[rd] = memory_content;
    }

    // internal cycle
    (*timer)++;
}

#ifndef INLINED_INCLUDES
};
#endif