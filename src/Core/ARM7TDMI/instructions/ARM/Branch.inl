/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 * */

#ifndef INLINED_INCLUDES
#include "../../ARM7TDMI.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

void BranchExchange(u32 instruction) {
    u8 rn = instruction & 0x0f;
    u32 target = Registers[rn];
    CPSR &= ~static_cast<u32>(CPSRFlags::T);
    CPSR |= (target & 1) ? static_cast<u32>(CPSRFlags::T) : 0;
    log_cpu_verbose("BX r%d (-> %x, %s state)", rn, target, (CPSR & static_cast<u32>(CPSRFlags::T)) ? "THUMB" : "ARM");

    if (static_cast<State>(target & 1) == State::ARM) {
        pc = target & 0xffff'fffc;
    }
    else {
        pc = target & 0xffff'fffe;
    }
    FlushPipeline();
}

template<bool Link>
void Branch(u32 instruction) {
    if constexpr(Link) {
        log_cpu_verbose("Branch with link");
        this->lr = this->pc - 4;
    }
    else {
        log_cpu_verbose("Branch");
    }

    i32 offset = (i32)(instruction << 8) >> 6;  // shifted left by 2 and sign extended

    this->pc += offset;
    this->FlushPipeline();
}

#ifndef INLINED_INCLUDES
};
#endif