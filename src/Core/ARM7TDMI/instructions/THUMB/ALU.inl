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

enum class ALUOperationsOpcode : u8 {
    AND = 0b0000,
    EOR = 0b0001,
    LSL = 0b0010,
    LSR = 0b0011,
    ASR = 0b0100,
    ADC = 0b0101,
    SBC = 0b0110,
    ROR = 0b0111,
    TST = 0b1000,
    NEG = 0b1001,
    CMP = 0b1010,
    CMN = 0b1011,
    ORR = 0b1100,
    MUL = 0b1101,
    BIC = 0b1110,
    MVN = 0b1111,
};

template<u8 op>
void ALUOperations(u16 instruction) {
    log_cpu_verbose("ALU ops %x", instruction);
    u8 rd = instruction & 7;
    u32 op1 = Registers[rd];
    u32 op2 = Registers[(instruction & 0x0038) >> 3];

    u32 result;
    const u32 carry = (CPSR & static_cast<u32>(CPSRFlags::C)) ? 1 : 0;
    switch (static_cast<ALUOperationsOpcode>(op)) {
        case ALUOperationsOpcode::AND:
            log_cpu_verbose("%08x AND %08x -> R%d", op1, op2, rd);
            result = op1 & op2;
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::EOR:
            log_cpu_verbose("%08x EOR %08x -> R%d", op1, op2, rd);
            result = op1 ^ op2;
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::LSL:
            log_cpu_verbose("%08x LSL %08x -> R%d", op1, op2, rd);
            if (likely(op2)) {
                result = DoShift<true, static_cast<u8>(ShiftType::LSL)>(op1, op2);
            }
            else {
                result = op1;
            }
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::LSR:
            log_cpu_verbose("%08x LSR %08x -> R%d", op1, op2, rd);
            if (likely(op2)) {
                result = DoShift<true, static_cast<u8>(ShiftType::LSR)>(op1, op2);
            }
            else {
                result = op1;
            }
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::ASR:
            log_cpu_verbose("%08x ASR %08x -> R%d", op1, op2, rd);
            if (likely(op2)) {
                result = DoShift<true, static_cast<u8>(ShiftType::ASR)>(op1, op2);
            }
            else {
                result = op1;
            }
            this->Registers[rd] = result;
            log_debug("%08x ASR %08x -> R%d [%x] @%x", op1, op2, rd, result, pc);
            break;
        case ALUOperationsOpcode::ADC:
            log_cpu_verbose("%08x ADC %08x -> R%d", op1, op2, rd);
            result = adcs_cv(op1, op2, carry);
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::SBC:
            log_cpu_verbose("%08x SBC %08x -> R%d", op1, op2, rd);
            result = sbcs_cv(op1, op2, carry);

            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::ROR:
            log_cpu_verbose("%08x ROR %08x -> R%d", op1, op2, rd);
            if (likely(op2)) {
                result = DoShift<true, static_cast<u8>(ShiftType::ROR)>(op1, op2);
            }
            else {
                result = op1;
            }
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::TST:
            log_cpu_verbose("%08x TST %08x (AND, no store)", op1, op2);
            result = op1 & op2;
            break;
        case ALUOperationsOpcode::NEG:
            log_cpu_verbose("     ROR %08x -> R%d", op2, rd);
            result = subs_cv(0, op2);
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::CMP:
            log_cpu_verbose("%08x CMP %08x (SUB, no store)", op1, op2);
            result = subs_cv(op1, op2);
            break;
        case ALUOperationsOpcode::CMN:
            log_cpu_verbose("%08x CMN %08x (ADD, no store)", op1, op2);
            result = adds_cv(op1, op2);
            break;
        case ALUOperationsOpcode::ORR:
            log_cpu_verbose("%08x ORR %08x -> R%d", op1, op2, rd);
            result = op1 | op2;
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::MUL:
            log_cpu_verbose("MUL %08x -> R%d", op2, rd);
            result = op1 * op2;
            // C and V are garbage
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::BIC:
            log_cpu_verbose("%08x BIC %08x -> R%d (op1 & ~op2)", op1, op2, rd);
            result = op1 & ~op2;
            this->Registers[rd] = result;
            break;
        case ALUOperationsOpcode::MVN:
            log_cpu_verbose("MVN %08x -> R%d", op2, rd);
            result = ~op2;
            this->Registers[rd] = result;
            break;
        default: UNREACHABLE
            log_fatal("Invalid opcode for ALU operations: %x", op);
    }

    SetNZ(result);
}

template<bool I, bool Op, u8 RnOff3>
void AddSubtract(u16 instruction) {
    log_cpu_verbose("AddSubtract %x", instruction);
    u32 operand;

    u8 rd = (instruction & 0x0007);
    u8 rs = (instruction & 0x0038) >> 3;

    if constexpr(I) {
        operand = RnOff3;
    }
    else {
        operand = Registers[RnOff3];
    }

    if constexpr(Op) {
        // subtract
        Registers[rd] = subs_cv(Registers[rs], operand);
    }
    else {
        // add
        Registers[rd] = adds_cv(Registers[rs], operand);
    }

    SetNZ(Registers[rd]);

    if constexpr(I) {
        // internal cycle
        timer++;
    }
}

enum class HiRegOp : u8{
    ADD = 0b00,
    CMP = 0b01,
    MOV = 0b10,
    BX  = 0b11,
};

template<u8 op, bool H1, bool H2>
void HiRegOps_BX(u16 instruction) {
    log_cpu_verbose("HiRegOps %x", instruction);
    u8 rd = (instruction & 0x0007);
    u8 rs = ((instruction & 0x0038) >> 3);

    if constexpr(H1) {
        rd += 8;
    }

    if constexpr(H2) {
        rs += 8;
    }

    u32 Rd = Registers[rd];
    u32 Rs = Registers[rs];

    switch(static_cast<HiRegOp>(op)) {
        case HiRegOp::ADD:
            Registers[rd] += Rs;
            if constexpr(H1) {
                // Rd == PC is only possible if H1 is set
                if (unlikely(rd == 15)) {
                    pc &= 0xffff'fffe;
                    FakePipelineFlush();
                }
            }
            return;
        case HiRegOp::CMP:
            // only opcode that sets condition codes in this category
            SetNZ(subs_cv(Rd, Rs));
            return;
        case HiRegOp::MOV:
            Registers[rd] = Rs;
            if constexpr(H1) {
                // Rd == PC is only possible if H1 is set
                if (unlikely(rd == 15)) {
                    pc &= 0xffff'fffe;
                    FakePipelineFlush();
                }
            }
            return;
        case HiRegOp::BX:
            // create a "fake BX instruction"
            // BX expects ssss (destination) in the bottom few bits
            BranchExchange(rs);
            return;
        default: UNREACHABLE
            log_fatal("Invalid HI register operations opcode: %x", op);
    }
}

template<u8 Op, u8 Offs5>
void MoveShifted(u16 instruction) {
    log_cpu_verbose("MOV SHFT Op=%d, Offs5=%x", Op, Offs5);
    u8 rd = (instruction & 7);
    u32 Rs = Registers[(instruction & 0x0038) >> 3];

    // fake an immediate shifted data processing operation
    Registers[rd] = ShiftByImmediate<true, Op>(Rs, Offs5);
    SetNZ(Registers[rd]);

    // internal cycle
    timer++;
}

template<u8 Op, u8 rd>
void ALUImmediate(u16 instruction) {
    log_cpu_verbose("ALU IMM Op=%d, rd=%d", Op, rd);
    u8 offs8 = (u8)instruction;
    u32 result;
    switch (Op) {
        case 0b00:  // MOV
            result = offs8;
            Registers[rd] = result;
            break;
        case 0b01:  // CMP
            result = subs_cv(Registers[rd], offs8);
            break;
        case 0b10:  // ADD
            result = adds_cv(Registers[rd], offs8);
            Registers[rd] = result;
            break;
        case 0b11:  // SUB
            result = subs_cv(Registers[rd], offs8);
            Registers[rd] = result;
            break;
    }
    SetNZ(result);

    // internal cycle
    timer++;
}

#ifndef INLINED_INCLUDES
};
#endif