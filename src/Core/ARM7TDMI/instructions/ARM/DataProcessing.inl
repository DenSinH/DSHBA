/*
 * INL files will be "inline" included in .cpp files
 * This allows us to do templated class functions easier
 *
 * We kind of "hack in" a fake class, the INLINED_INCLUDES macro is defined right before these files are included
 * This is so that IDEs know what is going on (sort of)
 * */

#ifndef INLINED_INCLUDES
#include "../../ARM7TDMI.h"
#include "helpers.h"

class ARM7TDMI_INL : ARM7TDMI {
#endif

enum class DataProcessingOpcode : u8 {
    AND = 0b0000,
    EOR = 0b0001,
    SUB = 0b0010,
    RSB = 0b0011,
    ADD = 0b0100,
    ADC = 0b0101,
    SBC = 0b0110,
    RSC = 0b0111,
    TST = 0b1000,
    TEQ = 0b1001,
    CMP = 0b1010,
    CMN = 0b1011,
    ORR = 0b1100,
    MOV = 0b1101,
    BIC = 0b1110,
    MVN = 0b1111,
};

enum class ShiftType : u8 {
    LSL = 0b00,
    LSR = 0b01,
    ASR = 0b10,
    ROR = 0b11,
};

template<bool S, u8 shift_type, bool shift_imm>
inline __attribute__((always_inline)) u32 GetShiftedRegister(u32 instruction) {
    // When the I bit is 0
    u32 shift_amount;
    u32 Rm;
    if ((instruction & 0xf) == 15) {
        // account for PC offset, should be 12, is 8
        Rm = pc + 4;
    }
    else {
        Rm = Registers[instruction & 0xf];
    }

    if constexpr(shift_imm) {
        // xSS0 pattern, bit 4 is 0
        shift_amount = (instruction & 0x0f80) >> 7;
    }
    else {
        shift_amount = Registers[(instruction & 0x0f00) >> 8];
    }

    if (!shift_amount) {
        switch (static_cast<ShiftType>(shift_type)) {
            case ShiftType::LSL:
                return Rm;
            case ShiftType::LSR:
            case ShiftType::ASR:
                shift_amount = 32;
                break;
            case ShiftType::ROR:
                // RRX
                if constexpr(S) {
                    u32 new_carry = Rm & 1;
                    Rm = (Rm >> 1) | (this->CPSR.C << 31);
                    this->CPSR.C = new_carry;
                    return Rm;
                }
                else {
                    return (Rm >> 1) | (this->CPSR.C << 31);
                }
            default:
                log_fatal("Invalid shift type: %d for shifting operand", shift_type);
        }
    }

    switch (static_cast<ShiftType>(shift_type)) {
        case ShiftType::LSL:
            if (shift_amount > 32) {
                if constexpr(S) {
                    this->CPSR.C = 0;
                }
                return 0;
            }
            else if (shift_amount == 32) {
                if constexpr(S) {
                    this->CPSR.C = Rm & 1;
                }
                return 0;
            }
            else {
                if constexpr(S) {
                    this->CPSR.C = (Rm & (0x10000'0000LLU >> shift_amount)) != 0;
                }
                return Rm << shift_amount;
            }
        case ShiftType::LSR:
            if (shift_amount > 32) {
                if constexpr(S) {
                    this->CPSR.C = 0;
                }
                return 0;
            }
            else if (shift_amount == 32) {
                if constexpr(S)  {
                    this->CPSR.C = Rm >> 31;
                }
                return 0;
            }
            else {
                if constexpr(S) {
                    this->CPSR.C = (Rm & (1 << (shift_amount - 1))) != 0;
                }
                return Rm >> shift_amount;
            }
        case ShiftType::ASR:
            if (shift_amount >= 32) {
                Rm = (Rm & 0x8000'0000) ? 0xffff'ffff : 0;
                if constexpr(S) {
                    this->CPSR.C = Rm & 1;
                }
                return Rm;
            }
            else {
                if constexpr(S) {
                    this->CPSR.C = (Rm & (1 << (shift_amount - 1))) != 0;
                }
                return Rm >> shift_amount;
            }
        case ShiftType::ROR:
            shift_amount &= 0x1f;
            if constexpr(S) {
                this->CPSR.C = (Rm & (1 << (shift_amount - 1)));
            }
            return ROTR32(Rm, shift_amount);
        default:
            log_fatal("Invalid shift type for shifted operand: %x", shift_type);
    }
}

template<u8 opcode, bool S>
inline __attribute__((always_inline)) void DoDataProcessing(u32 instruction, u32 op2) {
    u32 op1;
    if (((instruction & 0x000f0000) >> 16) == 15) {
        op1 = pc + 4;  // account for extra offset, same as before
    }
    else {
        op1 = Registers[(instruction & 0x000f0000) >> 16];
    }
    u32 Rd = (instruction & 0xf000) >> 12;

    u32 result, temp;
    switch (static_cast<DataProcessingOpcode>(opcode)) {
        case DataProcessingOpcode::AND:
            log_cpu_verbose("%08x AND %08x -> R%d", op1, op2, Rd);
            result = op1 & op2;
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::EOR:
            log_cpu_verbose("%08x EOR %08x -> R%d", op1, op2, Rd);
            result = op1 ^ op2;
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::SUB:
            log_cpu_verbose("%08x SUB %08x -> R%d", op1, op2, Rd);
            result = op1 - op2;
            if constexpr(S)
                SetCVSub(op1, op2, result);

            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::RSB:
            log_cpu_verbose("%08x RSB %08x (so %08x - %08x) -> R%d", op1, op2, Rd);
            result = op2 - op1;
            if constexpr(S)
                SetCVSub(op2, op1, result);

            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::ADD:
            log_cpu_verbose("%08x ADD %08x -> R%d", op1, op2, Rd);
            result = op1 + op2;
            if constexpr(S)
                SetCVAdd(op1, op2, result);
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::ADC:
            log_cpu_verbose("%08x ADC %08x -> R%d", op1, op2, Rd);
            result = op1 + op2 + CPSR.C;
            if constexpr(S)
                SetCVAddC(op1, op2, CPSR.C, result);

            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::SBC:
            log_cpu_verbose("%08x SBC %08x -> R%d", op1, op2, Rd);
            temp = op2 - CPSR.C + 1;
            result = (u32)(op1 - temp);
            if constexpr(S)
                SetCVSubC(op1, op2, CPSR.C, result);

            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::RSC:
            log_cpu_verbose("%08x RSC %08x -> R%d", op1, op2, Rd);
            temp = op1 - CPSR.C + 1;
            result = (u32)(op2 - temp);
            if constexpr(S)
                SetCVSubC(op2, op1, CPSR.C, result);

            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::TST:
            log_cpu_verbose("%08x TST %08x (AND, no store)", op1, op2);
            result = op1 & op2;
            break;
        case DataProcessingOpcode::TEQ:
            log_cpu_verbose("%08x TEQ %08x (EOR, no store)", op1, op2, Rd);
            result = op1 ^ op2;
            break;
        case DataProcessingOpcode::CMP:
            log_cpu_verbose("%08x CMP %08x (SUB, no store)", op1, op2, Rd);
            result = op1 - op2;
            if constexpr(S)
                SetCVSub(op1, op2, result);

            break;
        case DataProcessingOpcode::CMN:
            log_cpu_verbose("%08x CMN %08x (ADD, no store)", op1, op2, Rd);
            result = op1 + op2;
            if constexpr(S)
                SetCVAdd(op1, op2, result);

            break;
        case DataProcessingOpcode::ORR:
            log_cpu_verbose("%08x ORR %08x -> R%d", op1, op2, Rd);
            result = op1 | op2;
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::MOV:
            log_cpu_verbose("MOV %08x -> R%d", op2, Rd);
            result = op2;
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::BIC:
            log_cpu_verbose("%08x BIC %08x -> R%d (op1 & ~op2)", op1, op2, Rd);
            result = op1 & ~op2;
            this->Registers[Rd] = result;
            break;
        case DataProcessingOpcode::MVN:
            log_cpu_verbose("MVN %08x -> R%d", op2, Rd);
            result = ~op2;
            this->Registers[Rd] = result;
            break;
        default:
            log_fatal("Invalid opcode for DataProcessing operation: %x", opcode);
    }

    if constexpr(S) {
        SetNZ(result);
    }
}

/*
 * I OPCODE S fits in 27 - 20
 * xSS0 or 0SS1 fits in 7-4
 * this allows for some pretty intense specialization
 * */
template<bool I, u8 opcode, bool S, u8 shift_type>
void DataProcessing(u32 instruction) {
    log_cpu_verbose("Data Processing %08x (imm: %d, opcode: %x, S: %d, shift_type: %d)",
                    instruction, I, opcode, S, shift_type);

    u32 op2;
    u32 _Rd = (instruction & 0xf000) >> 12;
    if constexpr(I) {
        u32 imm = instruction & 0xff;
        u32 rot = (instruction & 0xf00) >> 7; // rotated right by twice the value
        op2 = ROTR32(imm, rot);
    }
    else {
        if constexpr(
                static_cast<DataProcessingOpcode>(opcode) == DataProcessingOpcode::ADC ||
                static_cast<DataProcessingOpcode>(opcode) == DataProcessingOpcode::SBC ||
                static_cast<DataProcessingOpcode>(opcode) == DataProcessingOpcode::RSC
        ) {
            // preserve old carry value
            op2 = GetShiftedRegister<false, shift_type, I>(instruction);
        }
        else {
            if (_Rd == 15) {
                // if Rd == PC, we don't set conditions either
                op2 = GetShiftedRegister<false, shift_type, I>(instruction);
            }
            else {
                op2 = GetShiftedRegister<S, shift_type, I>(instruction);
            }
        }
    }

    if (_Rd == 15) {
        DoDataProcessing<opcode, false>(instruction, op2);
        if constexpr(S) {
            // SPSR transfer
            this->CPSR = this->SPSR;
        }
    }
    else {
        DoDataProcessing<opcode, true>(instruction, op2);
    }

}

#ifndef INLINED_INCLUDES
};
#endif