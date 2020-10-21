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

template<bool P, bool U, bool S, bool W, bool L>
void BlockDataTransfer(u32 instruction) {
    u16 register_list = instruction & 0xffff;
    u8 Rn = (instruction & 0x000f'0000) >> 16;

    u32 original_address = Registers[Rn];
    u32 address = original_address;

    if constexpr(L) {
        log_cpu_verbose("LDM%c%c r%d%c {%x}", U ? 'I' : 'D', P ? 'B' : 'A', Rn, W ? '!' : ' ', register_list);
    }
    else {
        log_cpu_verbose("STM%c%c r%d%c {%x}", U ? 'I' : 'D', P ? 'B' : 'A', Rn, W ? '!' : ' ', register_list);
    }

    Mode old_mode = static_cast<Mode>(CPSR.Mode);
    if constexpr(S) {
        // PSR & force user
        ChangeMode(Mode::User);
    }

    if (!register_list) [[unlikely]] {
        // invalid register lists
        if constexpr(L) {
            pc = Memory->Mem::Read<u32, true>(address);
            FlushPipeline();
        }
        else {
            if constexpr(U) {
                // Up
                // If Pre-indexed: write at address + 4, otherwise at the starting address
                Memory->Mem::Write<u32, true>(address + (P ? 4 : 0), pc + 4);
            }
            else {
                // Down
                // more messed up, but same idea
                Memory->Mem::Write<u32, true>(address - (P ? 0x40 : 0x3c), pc + 4);
            }
        }
        if constexpr(W) {
            // writeback
            Registers[Rn] = U ? (address + 0x40) : (address - 0x40);
        }
    }
    else {
        // normal Rlists
        u32 rcount = popcount(register_list);
        if (!U) {
            // start stacking from the bottom of the register list
            address -= rcount << 2;

            // In this case, pre-decrement behaves like post-increment
            // therefore, after this point, if we want to check P, we should check P ^ !U
        }

        if constexpr(!L) {
            // Writeback with Rb included in Rlist: Store OLD base if Rb is FIRST entry in Rlist,
            // otherwise store NEW base (STM/ARMv4)
            // (GBATek)
            if ((register_list & ((1 << (Rn+ 1)) - 1)) == (1 << Rn)) {
                // This is only the case if Rn is the first register to be stored.
                // e.g.: if Rn is 4 and the Rlist ends in 0b11110000, we have
                // 0b11110000 & ((1 << 5) - 1) = 0b11110000 & (0b100000 - 1) =
                // 0b11110000 & 0b011111 = 0b00010000 = 1 << 4
                if constexpr(P ^ !U) {  // as I mentioned before, P ^ !U instead of P
                    // Pre-index
                    Memory->Mem::Write<u32, true>(address + 4, Registers[Rn]);
                }
                else {
                    // Post-index
                    Memory->Mem::Write<u32, true>(address, Registers[Rn]);
                }
                address += 4;
                original_address += U ? 4 : -4;  // for writeback

                // we already stored this one now
                register_list &= ~(1 << Rn);
                rcount--;
            }
        }

        // So now we want to write back the new base,
        // and since Rn might be overwritten by the load, we can just do that here
        if constexpr(W) {
            Registers[Rn] = U ? (original_address + (rcount << 2)) : (original_address - (rcount << 2));
        }

        for (unsigned i = 0; i < 16; i++) {
            if (register_list & (1 << i)) {
                if constexpr(P ^ !U) {
                    // "Pre-index"
                    address += 4;
                }

                if constexpr(L) {
                    Registers[i] = Memory->Mem::Read<u32, true>(address);
                }
                else {
                    Memory->Mem::Write<u32, false>(address, Registers[i]);
                }

                if constexpr(!(P ^ !U)) {
                    // "Post-index"
                    address += 4;
                }
            }
        }

        if (register_list & (1 << 15)) [[unlikely]] {
            if constexpr(L) {
                FlushPipeline();
            }
            else {
                // we wrote the value of PC + 8, while it should be + 12, so we correct for that now
                // in case of post-indexing, we need to subtract 4 again because we added this in the end
                Memory->Mem::Write<u32, true>(address - (!(P ^ !U) ? 4 : 0), pc + 4);
            }
        }
    }

    if constexpr(S) {
        // PSR & force user
        ChangeMode(old_mode);
    }

    if constexpr(L) {
        // internal cycle
        timer++;
    }
}

#ifndef INLINED_INCLUDES
};
#endif