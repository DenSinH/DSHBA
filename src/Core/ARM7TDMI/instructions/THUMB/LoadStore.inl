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
    log_cpu_verbose("L/SH L=%d, Offs5=%x", L, Offs5);
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
        Memory->Mem::Write<u16, true, true>(address, (u16)Registers[rd]);
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
            Memory->Mem::Write<u8, true, true>(address, (u8)Registers[rd]);
        }
        else {
            // STR
            Memory->Mem::Write<u32, true, true>(address, Registers[rd]);
        }
    }
}

template<bool L, bool B, u8 ro>
void LoadStoreRegOffs(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;
    log_cpu_verbose("L/SH [r%d, r%d] L=%d, B=%d", rb, ro, L, B);

    u32 address = Registers[rb] + Registers[ro];

    DoLoadStoreBL<B, L>(rd, address);
}

template<bool B, bool L, u8 Offs5>
void LoadStoreImmOffs(u16 instruction) {
    u8 rd = (instruction & 0x7);
    u8 rb = (instruction & 0x38) >> 3;
    log_cpu_verbose("L/S [r%d, #%x] B=%d, L=%d", rb, Offs5, B, L);

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
    log_cpu_verbose("L/SSB/H ro=%d H=%d, S=%d", ro, H, S);

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
            Memory->Mem::Write<u16, true, true>(address, Registers[rd]);
        }
    }
}

template<bool L, bool R>
void PushPop(u16 instruction) {
    u16 rlist = instruction & 0xff;
    log_cpu_verbose("Push/Pop L=%d, R=%d pc=%x", L, R, pc);

    if constexpr(L) {
        // pop
#ifndef HAS_CTTZ
        for (unsigned i = 0; i < 8; i++) {
#else
        for (unsigned i = cttz(rlist); i < 8; i++) {
#endif
            if (rlist & (1 << i)) {
                Registers[i] = Memory->Mem::Read<u32, true>(sp);
                sp += 4;
            }
        }

        if constexpr(R) {
            // load PC/LR
            pc = Memory->Mem::Read<u32, true>(sp) & ~1;  // make sure it's aligned
            sp += 4;
            FakePipelineFlush();
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
            Memory->Mem::Write<u32, true, true>(sp, lr);
        }

#if !defined(CTTZ_LDM_STM_LOOP_BASE) || !defined(HAS_CTTZ)
        for (int i = 7; i >= 0; i--) {
#else
        const int base = cttz(rlist);
        for (int i = 7; i >= base; i--) {
#endif
            if (rlist & (1 << i)) {
                sp -= 4;
                Memory->Mem::Write<u32, true, true>(sp, Registers[i]);
            }
        }
    }
}

template<bool L, u8 rb>
void MultipleLoadStore(u16 instruction) {
    u8 rlist = (u8)instruction;
    u32 address = Registers[rb];
    log_cpu_verbose("LDM L=%d, rb=%d", L, rb);

    if (unlikely(!rlist)) {
        // invalid register list
        if constexpr(L) {
            pc = Memory->Mem::Read<u32, true>(address);
            FakePipelineFlush();

            // internal cycle
            timer++;
        }
        else {
            Memory->Mem::Write<u32, true, true>(address, pc + 2);  // PC is 4 ahead, should be 6
        }

        Registers[rb] += 0x40;
        return;
    }

    if constexpr(L) {
#if !defined(CTTZ_LDM_STM_LOOP_BASE) || !defined(HAS_CTTZ)
        for (unsigned i = 0; i < 8; i++) {
#else
        for (unsigned i = cttz(rlist); i < 8; i++) {
#endif
            if (rlist & (1 << i)) {
                Registers[i] = Memory->Mem::Read<u32, true>(address);
                address += 4;
            }
        }
        Registers[rb] = address;

        // internal cycle
        timer++;
    }
    else {
        // Writeback with Rb in RList:
        // Store OLD base if Rb is FIRST entry in Rlist, otherwise store NEW base (STM/ARMv4)
#ifdef HAS_CTTZ
        if (unlikely(cttz(rlist) == rb)) {
#else
        if (unlikely(!(rlist & ((1 << rb) - 1))) {
#endif
            // This is only the case if rn is the first register to be stored.
            // e.g.: if rn is 4 and the Rlist ends in 0b11110000, we have
            // 0b11110000 & ((1 << 5) - 1) = 0b11110000 & (0b100000 - 1) =
            // 0b11110000 & 0b011111 = 0b00010000 = 1 << 4

            Memory->Mem::Write<u32, true, true>(address, Registers[rb]);
            address += 4;

            // we already stored this one now
            rlist &= ~(1 << rb);
        }

        // new base:
        Registers[rb] = (address + (popcount(rlist) << 2));

#if !defined(CTTZ_LDM_STM_LOOP_BASE) || !defined(HAS_CTTZ)
        for (unsigned i = 0; i < 8; i++) {
#else
        for (unsigned i = cttz(rlist); i < 8; i++) {
#endif
            if (rlist & (1 << i)) {
                Memory->Mem::Write<u32, true, true>(address, Registers[i]);
                address += 4;
            }
        }
    }
}

template<u8 rd>
void PCRelativeLoad(u16 instruction) {
    log_cpu_verbose("PCREL rd=%d", rd);
    u32 address = (pc & ~3) + ((instruction & 0xff) << 2);

    // loads this way will always be word aligned
    Registers[rd] = Memory->Mem::Read<u32, true>(address);

    // internal cycle
    timer++;
}

template<bool L, u8 rd>
void SPRelativeLoadStore(u16 instruction) {
    u16 word8 = (instruction & 0xff) << 2;

    if constexpr(L) {
        u32 loaded = Memory->Read<u32, true>(sp + word8);
        // word8 is always aligned, so we just check sp for misalignment

        u32 misalignment = sp & 3;
        if (unlikely(misalignment)) {
            loaded = ROTR32(loaded, misalignment << 3);
        }

        Registers[rd] = loaded;

        // internal cycle
        timer++;
    }
    else {
        Memory->Write<u32, true, true>(sp + word8, Registers[rd]);
    }
}

#ifndef INLINED_INCLUDES
};
#endif