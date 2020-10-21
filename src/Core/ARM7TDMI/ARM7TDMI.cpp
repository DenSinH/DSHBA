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
            return this->CPSR.Z != 0u;
        case Condition::NE:
            return this->CPSR.Z == 0u;
        case Condition::CS:
            return this->CPSR.C != 0u;
        case Condition::CC:
            return this->CPSR.C == 0u;
        case Condition::MI:
            return this->CPSR.N != 0u;
        case Condition::PL:
            return this->CPSR.N == 0u;
        case Condition::VS:
            return this->CPSR.V != 0u;
        case Condition::VC:
            return this->CPSR.V == 0u;
        case Condition::HI:
            return (this->CPSR.C != 0u) && (this->CPSR.Z == 0u);
        case Condition::LS:
            return (this->CPSR.C == 0u) || (this->CPSR.Z != 0u);
        case Condition::GE:
            return this->CPSR.V == this->CPSR.N;
        case Condition::LT:
            return this->CPSR.V != this->CPSR.N;
        case Condition::GT:
            return (this->CPSR.Z == 0u) && (this->CPSR.V == this->CPSR.N);
        case Condition::LE:
            return (this->CPSR.Z != 0u) || (this->CPSR.V != this->CPSR.N);
        case Condition::AL:
            return true;
        default:
            log_fatal("Invalid condition code: %x", condition);
    }
}

void ARM7TDMI::Step() {
    if (!this->CPSR.T) {
        // ARM mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, true>(this->pc));

        u32 instruction = this->Pipeline.Dequeue();
        if (CheckCondition(instruction >> 28)) {
            (this->*ARMInstructions[ARMHash(instruction)])(instruction);
        }

        this->pc += 4;
    }
    else {
        // THUMB mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, true>(this->pc));

        u16 instruction = this->Pipeline.Dequeue();
        (this->*THUMBInstructions[THUMBHash(instruction)])(instruction);

        this->pc += 2;
    }

    // for now, tick every instruction
    // todo: proper timings
    this->timer++;
}

void ARM7TDMI::FlushPipeline() {
    this->Pipeline.Clear();

    if (!this->CPSR.T) {
        // ARM mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, true>(this->pc));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u32, true>(this->pc + 4));
        this->pc += 4;
    }
    else {
        // THUMB mode
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, true>(this->pc));
        this->Pipeline.Enqueue(this->Memory->Mem::Read<u16, true>(this->pc + 2));
        this->pc += 2;
    }
}

void ARM7TDMI::BuildARMTable() {
    for (size_t i = 0; i < ARMInstructionTableSize; i++) {
        switch ((i & 0xc00) >> 10) {
            case 0b00:
            case 0b01:
                break;
            case 0b10:
                if ((i & (ARMHash(0x0e00'0000))) == ARMHash(0x0a00'0000)) {
                    if (i & ARMHash(0x0100'0000)) {
                        // Link bit
                        ARMInstructions[i] = &ARM7TDMI::Branch<true>;
                    }
                    else {
                        ARMInstructions[i] = &ARM7TDMI::Branch<false>;
                    }
                    continue;
                }
                break;
            case 0b11:
                break;
            default:
                break;
        }
        this->ARMInstructions[i] = &ARM7TDMI::ARMUnimplemented;
    }
}

void ARM7TDMI::BuildTHUMBTable() {
    for (size_t i = 0; i < THUMBInstructionTableSize; i++) {
        this->THUMBInstructions[i] = &ARM7TDMI::THUMBUnimplemented;
    }
}