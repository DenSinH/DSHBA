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

template<u8 cond>
THUMB_INSTRUCTION(ConditionalBranch) {
    FlushNZ();  // calculate NZ flags
    if (CheckCondition(cond)) {
        i32 offset = (i32)((i8)((u8)instruction)) << 1;

#ifdef BASIC_IDLE_DETECTION
        if (unlikely(offset == -4)) {
            // We know we are in THUMB mode
            Idle();
        }
#endif
        pc += offset;
        FakePipelineFlush<false>();
    }
}

template<bool H>
THUMB_INSTRUCTION(LongBranchLink) {
    // part of a bigger instruction (H bit clear, H bit set)
    u16 offset = (instruction & 0x07ff);

    if constexpr(!H) {
        // First Instruction - LR = PC+4+(nn SHL 12)
        // PC + 4 is just because it's 2 instructions ahead in the above description
        i16 signed_offset = (i16)(offset << 5) >> 5;
        lr = pc + ((i32)signed_offset << 12);
    }
    else {
        // Second Instruction - PC = LR + (nn SHL 1), and LR = PC+2 OR 1
        u32 old_pc = pc - 2;  // correct for pipeline effects
        pc = lr + (offset << 1);
        // todo: idle loop detection?

        lr = old_pc | 1;
        FakePipelineFlush<false>();
    }
}

THUMB_INSTRUCTION(UnconditionalBranch) {
    i32 offs11 = (instruction & 0x7ff);
    offs11 = (offs11 << 21) >> 20;  // sign extend and shift by 1

    pc += offs11;
    FakePipelineFlush<false>();
#ifdef BASIC_IDLE_DETECTION
    if (unlikely(offs11 == -4)) {
        // We know we are in THUMB mode
        Idle();
    }
#endif
}

#ifndef INLINED_INCLUDES
};
#endif