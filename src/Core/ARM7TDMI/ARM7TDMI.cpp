#include "ARM7TDMI.h"

void ARM7TDMI::Step() {
    u32 instruction;
//    u32 old_pc = pc;

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
        log_cpu_verbose("PC pre: %x", pc);
        if (CheckCondition(instruction >> 28)) {
            (this->*ARMInstructions[ARMHash(instruction)])(instruction);
        }

        // we handle mode changes in the BX instruction by correcting PC for it there
        // this saves us from doing another check after every instruction
        pc += 4;
        log_cpu_verbose("PC post: %x", pc);
    }
    else {
        // THUMB mode
        log_cpu_verbose("PC pre t: %x", pc);
        (this->*THUMBInstructions[THUMBHash((u16)instruction)])((u16)instruction);
        // same here
        pc += 2;
        log_cpu_verbose("PC post t: %x", pc);
    }

//    if (pc & 1) {
//        log_fatal("Odd pc: %x -> %x", old_pc, pc);
//    }

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
    log_warn("Reflush for write near PC");

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