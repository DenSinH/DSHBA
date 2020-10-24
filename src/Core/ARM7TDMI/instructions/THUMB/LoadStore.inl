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

template<bool L, u8 Offs5>
void LoadStoreHalfword(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;

    u32 address = Registers[rb] + (Offs5 << 1);

    if constexpr(L) {
        // Load
        if (unlikely(address & 1)) {
            // misaligned
            Registers[rd] = ROTR32(Memory->Mem::Read<u16, true>(address), 8);
        }
        else {
            Registers[rd] = Memory->Mem::Read<u16, true>(address);
        }

        // internal cycle
        timer++;
    }
    else {
        Memory->Mem::Write<u16, true>(address, (u16)Registers[rd]);
    }
}

template<bool B, bool L>
ALWAYS_INLINE void DoLoadStoreBL(u8 rd, u32 address) {
    if constexpr(L) {
        // Load
        if constexpr(B) {
            // LDRB
            Registers[rd] = Memory->Mem::Read<u8, true>(address);
        }
        else {
            // LDR
            u8 rotate_amount = (address & 3) << 3;
            if (unlikely(rotate_amount)) {
                Registers[rd] = ROTR32(Memory->Mem::Read<u32, true>(address), rotate_amount);
            }
            else {
                Registers[rd] = Memory->Mem::Read<u32, true>(address);
            }
        }
    }
    else {
        // Store
        if constexpr(B) {
            // STRB
            Memory->Mem::Write<u8, true>(address, (u8)Registers[rd]);
        }
        else {
            // STR
            Memory->Mem::Write<u32, true>(address, Registers[rd]);
        }
    }
}

template<bool L, bool B, u8 ro>
void LoadStoreRegOffs(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;

    u32 address = Registers[rb] + Registers[ro];

    DoLoadStoreBL<B, L>(rd, address);
}

template<bool B, bool L, u8 Offs5>
void LoadStoreImmOffs(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;

    u32 address;
    if constexpr(B) {
        address = Registers[rb] + Offs5;
    }
    else {
        address = Registers[rb] + (Offs5 << 2);
    }

    DoLoadStoreBL<B, L>(rd, address);
}

template<bool H, bool S, u8 ro>
void LoadStoreSEBH(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;

    u32 address = Registers[rb] + Registers[ro];

    if constexpr(S) {
        if constexpr(H) {
            // LDRSH
            if (unlikely(address & 1)) {
                // misaligned, sign extended byte is loaded instead
                Registers[rd] = (u32)((i32)((i8)Memory->Mem::Read<u8, true>(address)));
            }
            else {
                Registers[rd] = (u32)((i32)((i16)Memory->Mem::Read<u16, true>(address)));
            }
        }
        else {
            // LDRSB
            Registers[rd] = (u32)((i32)((i8)Memory->Mem::Read<u8, true>(address)));
        }

        // internal cycle
        timer++;
    }
    else {
        if constexpr(H) {
            // LDRH
            if (unlikely(address & 1)) {
                // misaligned, rotated value is loaded
                Registers[rd] = ROTR32(Memory->Mem::Read<u16, true>(address), 8);
            }
            else {
                Registers[rd] = Memory->Mem::Read<u16, true>(address);
            }

            // internal cycle
            timer++;
        }
        else {
            // STRH
            Memory->Mem::Write<u16, true>(address, Registers[rd]);
        }
    }
}

template<bool L, bool R>
void PushPop(u16 instruction) {
    u16 rlist = instruction & 0xff;

    if constexpr(L) {
        // pop
        for (u32 i = 0; i < 8; i++) {
            if (rlist & (1 << i)) {
                Registers[i] = Memory->Mem::Read<u32, true>(sp);
                sp += 4;
            }
        }

        if constexpr(R) {
            // load PC/LR
            pc = Memory->Mem::Read<u32, true>(sp);
            sp += 4;
            FlushPipeline();
        }

        // internal cycle
        timer++;
    }
    else {
        // push
        // pushing happens bottom up, like in ARM STM
        if constexpr(R) {
            // push LR
            sp -= 4;
            Memory->Mem::Write<u32, true>(sp, lr);
            FlushPipeline();
        }

        for (int i = 7; i >= 0; i--) {
            if (rlist & (1 << i)) {
                sp -= 4;
                Memory->Mem::Write<u32, true>(sp, Registers[i]);
            }
        }
    }
}

#ifndef INLINED_INCLUDES
};
#endif