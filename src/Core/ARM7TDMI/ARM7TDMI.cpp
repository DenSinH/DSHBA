#include "ARM7TDMI.h"

#ifdef TRACE_LOG
#include <iomanip>
#endif

void ARM7TDMI::Step() {
    u32 instruction;
//    u32 old_pc = pc;

#ifdef TRACE_LOG
    LogState();
#endif

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

#ifdef TRACE_LOG
void ARM7TDMI::LogState() {
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[0] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[1] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[2] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[3] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[4] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[5] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[6] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[7] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[8] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[9] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[10] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[11] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[12] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[13] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[14] << " ";
    trace << std::setfill('0') << std::setw(8) << std::hex << Registers[15] - ((CPSR & static_cast<u32>(CPSRFlags::T)) ? 2 : 4) << " ";
    trace << "cpsr: ";
    trace << std::setfill('0') << std::setw(8) << std::hex << CPSR << std::endl;
}
#endif