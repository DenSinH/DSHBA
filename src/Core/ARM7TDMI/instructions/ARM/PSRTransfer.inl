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

template<bool Ps>
void __fastcall MRS(u32 instruction) {
    // PSR -> reg
    u8 rd = (instruction & 0xf000) >> 12;

    if constexpr(Ps) {
        log_cpu_verbose("MRS r%d, SPSR", rd);
        Registers[rd] = SPSR;
    }
    else {
        log_cpu_verbose("MRS r%d, CPSR", rd);
        Registers[rd] = CPSR;
    }
}

static constexpr u32 mask[16] = {
    0x0000'0000, 0x0000'00ff, 0x0000'ff00, 0x0000'ffff,
    0x00ff'0000, 0x00ff'00ff, 0x00ff'ff00, 0x00ff'ffff,
    0xff00'0000, 0xff00'00ff, 0xff00'ff00, 0xff00'ffff,
    0xffff'0000, 0xffff'00ff, 0xffff'ff00, 0xffff'ffff,
};

template<bool I, bool Pd>
void __fastcall MSR(u32 instruction) {
    // reg/imm -> PSR
    /*
     * Not described in the manual, but from GBATek:
     *     bit:
     *      19      f  write to flags field     Bit 31-24 (aka _flg)
     *      18      s  write to status field    Bit 23-16 (reserved, don't change)
     *      17      x  write to extension field Bit 15-8  (reserved, don't change)
     *      16      c  write to control field   Bit 7-0   (aka _ctl)
     * */
    u32 operand;
    u8 flags = (instruction & 0x000f'0000) >> 16;

    if constexpr(I) {
        // immediate value
        u32 imm = instruction & 0xff;
        u32 rot = (instruction & 0x0f00) >> 7;
        operand = ROTR32(imm, rot);
        log_cpu_verbose("MSR %cPSR, #%x", Pd ? 'S' : 'C', operand);
    }
    else {
        operand = Registers[instruction & 0xf];
        log_cpu_verbose("MSR %cPSR, r%d", Pd ? 'S' : 'C', operand);
    }

    if constexpr(Pd) {
        SPSR = (SPSR & ~mask[flags]) | (operand & mask[flags]);
    }
    else {
        if (flags & 1) {
            // mode might change
            // since ChangeMode returns if we are in the same mode, do this before actually changing the mode
            ChangeMode(static_cast<Mode>(operand & static_cast<u32>(CPSRFlags::Mode)));
        }
        CPSR = (CPSR & ~mask[flags]) | (operand & mask[flags]);
        if (CPSR & static_cast<u32>(CPSRFlags::T)) {
            // entered THUMB mode, correct pc and reload LastNZ
            pc -= 2;
            SetLastNZ();
            ARMMode = false;
        }

        // I flag might have been set
        ScheduleInterruptPoll();
    }
}

#ifndef INLINED_INCLUDES
};
#endif