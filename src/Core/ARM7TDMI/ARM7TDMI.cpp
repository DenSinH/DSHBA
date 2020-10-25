#include "ARM7TDMI.h"

enum class Condition : u8 {
    EQ = 0b0000,
    NE = 0b0001,
    CS = 0b0010,
    CC = 0b0011,
    MI = 0b0100,
    PL = 0b0101,
    VS = 0b0110,
    VC = 0b0111,
    HI = 0b1000,
    LS = 0b1001,
    GE = 0b1010,
    LT = 0b1011,
    GT = 0b1100,
    LE = 0b1101,
    AL = 0b1110,
};

bool ARM7TDMI::CheckCondition(u8 condition) const {
    switch (static_cast<Condition>(condition)) {
        case Condition::EQ:
            return (CPSR & static_cast<u32>(CPSRFlags::Z)) != 0;
        case Condition::NE:
            return (CPSR & static_cast<u32>(CPSRFlags::Z)) == 0;
        case Condition::CS:
            return (CPSR & static_cast<u32>(CPSRFlags::C)) != 0;
        case Condition::CC:
            return (CPSR & static_cast<u32>(CPSRFlags::C)) == 0;
        case Condition::MI:
            return (CPSR & static_cast<u32>(CPSRFlags::N)) != 0;
        case Condition::PL:
            return (CPSR & static_cast<u32>(CPSRFlags::N)) == 0;
        case Condition::VS:
            return (CPSR & static_cast<u32>(CPSRFlags::V)) != 0;
        case Condition::VC:
            return (CPSR & static_cast<u32>(CPSRFlags::V)) == 0;
        case Condition::HI:
            return ((CPSR & static_cast<u32>(CPSRFlags::C)) != 0u) && ((CPSR & static_cast<u32>(CPSRFlags::Z)) == 0u);
        case Condition::LS:
            return ((CPSR & static_cast<u32>(CPSRFlags::C)) == 0u) || ((CPSR & static_cast<u32>(CPSRFlags::Z)) != 0u);
        case Condition::GE:
            return ((CPSR & static_cast<u32>(CPSRFlags::V)) != 0) == ((CPSR & static_cast<u32>(CPSRFlags::N)) != 0);
        case Condition::LT:
            return ((CPSR & static_cast<u32>(CPSRFlags::V)) != 0) ^ ((CPSR & static_cast<u32>(CPSRFlags::N)) != 0);
        case Condition::GT:
            return (!(CPSR & static_cast<u32>(CPSRFlags::Z))) &&
                    (((CPSR & static_cast<u32>(CPSRFlags::V)) != 0) == ((CPSR & static_cast<u32>(CPSRFlags::N)) != 0));
        case Condition::LE:
            return ((CPSR & static_cast<u32>(CPSRFlags::Z)) != 0) ||
                   (((CPSR & static_cast<u32>(CPSRFlags::V)) != 0) ^ ((CPSR & static_cast<u32>(CPSRFlags::N)) != 0));
        case Condition::AL:
            return true;
        default:
            log_fatal("Invalid condition code: %x", condition);
    }
}

void ARM7TDMI::Step() {
    u32 instruction;
    bool ARMMode = !(CPSR & static_cast<u32>(CPSRFlags::T));
    if (unlikely(Pipeline.Count)) {
        // we only have stuff in the pipeline if writes near PC happened
        instruction = Pipeline.Dequeue();
    }
    else if (ARMMode) {
        // before the instruction gets executed, we are 2 instructions ahead
        instruction = Memory->Mem::Read<u32, true>(pc - 8);
    }
    else {
        // THUMB mode
        instruction = Memory->Mem::Read<u16, true>(pc - 4);
    }

    if (ARMMode) {
        // ARM mode
        if (CheckCondition(instruction >> 28)) {
            (this->*ARMInstructions[ARMHash(instruction)])(instruction);
        }

        // we handle mode changes in the BX instruction by correcting PC for it there
        // this saves us from doing another check after every instruction
        pc += 4;
    }
    else {
        // THUMB mode
        (this->*THUMBInstructions[THUMBHash((u16)instruction)])((u16)instruction);

        // same here
        pc += 2;
    }

    // for now, tick every instruction
    // todo: proper timings (in memory accesses)
    this->timer++;
}

void ARM7TDMI::FakePipelineFlush() {
    this->Pipeline.Clear();
    // todo: fake memory timings for pipeline fills

    if (!(CPSR & static_cast<u32>(CPSRFlags::T))) {
        // ARM mode
        this->pc += 4;
    }
    else {
        // THUMB mode
        this->pc += 2;
    }
}

void ARM7TDMI::PipelineReflush() {
    this->Pipeline.Clear();
    // if instructions that should be in the pipeline get written to
    // PC is in an instruction when this happens (marked by a bool in the template)

    if (!(CPSR & static_cast<u32>(CPSRFlags::T))) {
        // ARM mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, true>(this->pc - 4));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, true>(this->pc));
    }
    else {
        // THUMB mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, true>(this->pc - 2));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, true>(this->pc));
    }
}