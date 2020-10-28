/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 *
 * We kind of "hack in" a fake class, the INLINED_INCLUDES macro is defined right before these files are included
 * This is so that IDEs know what is going on (sort of)
 * */

#ifndef INLINED_INCLUDES
#include "../ARM7TDMI.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

template<typename T>
void SWI(T instruction) {
    SPSRBank[static_cast<u32>(Mode::Supervisor) & 0xf] = CPSR;
    ChangeMode(Mode::Supervisor);
    CPSR |= static_cast<u32>(CPSRFlags::I);

    // LR_svc holds address or instruction that was not executed
    // we are now in svc mode, so lr is lr_svc
    if constexpr(std::is_same_v<T, u32>) {
        // ARM mode
        lr = pc - 4;
        // we are already in ARM mode
        pc = static_cast<u32>(ExceptionVector::SWI);
    }
    else {
        // THUMB mode
        lr = pc - 2;
        // enter ARM mode
        CPSR &= ~static_cast<u32>(CPSRFlags::T);

        // since we add 2 after an instruction in THUMB mode to stay ahead, we need to correct for this,
        // as we switched into ARM mode
        pc = static_cast<u32>(ExceptionVector::SWI) + 2;
    }

    FakePipelineFlush();
}

#ifndef INLINED_INCLUDES
};
#endif