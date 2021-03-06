/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 * */

#ifndef INLINED_INCLUDES
#include "../../ARM7TDMI.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

INSTRUCTION(BranchExchange) {
    u8 rn = instruction & 0x0f;
    u32 target = Registers[rn];
    bool was_thumb = (CPSR & static_cast<u32>(CPSRFlags::T)) != 0;

    CPSR &= ~static_cast<u32>(CPSRFlags::T);
    CPSR |= (target & 1) ? static_cast<u32>(CPSRFlags::T) : 0;
    log_cpu_verbose("BX r%d (-> %x, %s state)", rn, target, (CPSR & static_cast<u32>(CPSRFlags::T)) ? "THUMB" : "ARM");

    if (!(target & 1)) {
        // enter ARM mode
#ifdef BASIC_IDLE_DETECTION
        u32 instruction_address = pc - 8;
#endif
        pc = target & 0xffff'fffc;

        if (was_thumb) {
            // reflush NZ
            FlushNZ();

            // enter ARM mode
            ARMMode = true;

            // correct for adding 2 after instruction because we were in THUMB mode
            pc += 2;
        }
#ifdef BASIC_IDLE_DETECTION
        else if (unlikely(target == instruction_address)) {
            // we can only really detect idle branches if we don't change modes
            Idle();
        }
#endif
        // for fake pipeline flushes, it does not matter that PC has been altered
        FakePipelineFlush<true>();
    }
    else {
        // enter THUMB mode
#ifdef BASIC_IDLE_DETECTION
        u32 instruction_address = pc - 4;
#endif
        pc = target & 0xffff'fffe;

        if (!was_thumb) {
            // correct for adding 4 after instruction because we were in ARM mode
            pc -= 2;

            // set default NZ values
            SetLastNZ();
            ARMMode = false;
        }
#ifdef BASIC_IDLE_DETECTION
        else if (unlikely(target == instruction_address)) {
            // we can only really detect idle branches if we don't change modes
            Idle();
        }
#endif
        FakePipelineFlush<false>();
    }
}

template<bool Link>
INSTRUCTION(Branch) {
    if constexpr(Link) {
        log_cpu_verbose("Branch with link");
        this->lr = this->pc - 4;
    }
    else {
        log_cpu_verbose("Branch");
    }

    i32 offset = (i32)(instruction << 8) >> 6;  // shifted left by 2 and sign extended

#ifdef BASIC_IDLE_DETECTION
    if (unlikely(offset == -8)) {
        // We know we are in ARM mode
        Idle();
    }
#endif

    this->pc += offset;
    this->FakePipelineFlush<true>();
}

#ifndef INLINED_INCLUDES
};
#endif