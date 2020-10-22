#include "ARM7TDMI.h"
#include "CoreUtils.h"

template<u32 instruction>
static constexpr ARMInstructionPtr GetARMInstruction() {
    switch ((instruction & 0xc00) >> 10) {
        case 0b00:
        {
            if constexpr((instruction & ARM7TDMI::ARMHash(0x0fc0'00f0)) == ARM7TDMI::ARMHash(0x0000'0090)) {
                // multiply
                break;
            }
            else if constexpr((instruction & ARM7TDMI::ARMHash(0x0f80'00f0)) == ARM7TDMI::ARMHash(0x0080'0090)) {
                // multiply long
                break;
            }
            else if constexpr((instruction & ARM7TDMI::ARMHash(0x0fb0'0ff0)) == ARM7TDMI::ARMHash(0x0100'0090)) {
                // SWP
                break;
            }
            else if constexpr(instruction == 0b0001'0010'0001) {
                // BX
                return &ARM7TDMI::BranchExchange;
            }
            else if constexpr((instruction & ARM7TDMI::ARMHash(0x0e00'0090)) == ARM7TDMI::ARMHash(0x0000'0090)) {
                // Halfword Data Transfer Imm/Reg Offset
                const bool P = (instruction & ARM7TDMI::ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARM7TDMI::ARMHash(0x0080'0000)) != 0;
                const bool imm_offset = (instruction & ARM7TDMI::ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARM7TDMI::ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARM7TDMI::ARMHash(0x0010'0000)) != 0;
                const bool S = (instruction & ARM7TDMI::ARMHash(0x0000'0040)) != 0;
                const bool H = (instruction & ARM7TDMI::ARMHash(0x0000'0020)) != 0;
                return &ARM7TDMI::HalfwordDataTransfer<P, U, imm_offset, W, L, S, H>;
            }
            else {
                // Data Processing or PSR transfer
                const u8 opcode = (instruction >> 5) & 0xf;
                const bool S = (instruction & ARM7TDMI::ARMHash(0x0010'0000)) != 0;

                if constexpr(!S && (0b1000 <= opcode) && (opcode <= 0b1011)) {
                    // PSR transfer
                    const bool P = (instruction & ARM7TDMI::ARMHash(0x0040'0000)) != 0;
                    if constexpr((instruction & (ARM7TDMI::ARMHash(0x0fb0'00f0))) == ARM7TDMI::ARMHash(0x0100'0000)) {
                        // MRS (PSR -> regs)
                        return &ARM7TDMI::MRS<P>;
                    }
                    else {
                        // MSR (reg/imm -> PSR)
                        const bool I = (instruction & ARM7TDMI::ARMHash(0x0200'0000)) != 0;
                        return &ARM7TDMI::MSR<I, P>;
                    }
                }
                else {
                    const bool I = (instruction & ARM7TDMI::ARMHash(0x0200'0000)) != 0;
                    const u8 shift_type = (instruction & 6) >> 1;
                    // has to be == 0, I messed this up, but this is the way it is now...
                    // doesn't make much of a difference in the end, its nicer to use true for immediate shift anyway,
                    // cause I need to use it in ldr/str anyway
                    const bool shift_imm = (instruction & ARM7TDMI::ARMHash(0x0000'0010)) == 0;

                    if constexpr((0b1000 <= opcode) && (opcode <= 0b1011)) {
                        // S bit is always set on these
                        return &ARM7TDMI::DataProcessing<I, opcode, true, shift_type, shift_imm>;
                    }
                    else {
                        return &ARM7TDMI::DataProcessing<I, opcode, S, shift_type, shift_imm>;
                    }
                }
            }
        }
        case 0b01:
            if constexpr((instruction & (ARM7TDMI::ARMHash(0x0e00'0010))) == ARM7TDMI::ARMHash(0x0600'0010)) {
                // undefined
                return &ARM7TDMI::ARMUnimplemented;
            }
            else {
                // Single Data Transfer
                const bool I = (instruction & ARM7TDMI::ARMHash(0x0200'0000)) != 0;
                const bool P = (instruction & ARM7TDMI::ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARM7TDMI::ARMHash(0x0080'0000)) != 0;
                const bool B = (instruction & ARM7TDMI::ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARM7TDMI::ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARM7TDMI::ARMHash(0x0010'0000)) != 0;
                const u8 shift_type = (instruction & 6) >> 1;

                return &ARM7TDMI::SingleDataTransfer<I, P, U, B, W, L, shift_type>;
            }
        case 0b10:
            if constexpr((instruction & (ARM7TDMI::ARMHash(0x0e00'0000))) == ARM7TDMI::ARMHash(0x0a00'0000)) {
                // Branch (with Link)
                return &ARM7TDMI::Branch<(instruction & ARM7TDMI::ARMHash(0x0100'0000)) != 0>;
            }
            else {
                // Block Data Transfer
                const bool P = (instruction & ARM7TDMI::ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARM7TDMI::ARMHash(0x0080'0000)) != 0;
                const bool S = (instruction & ARM7TDMI::ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARM7TDMI::ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARM7TDMI::ARMHash(0x0010'0000)) != 0;
                return &ARM7TDMI::BlockDataTransfer<P, U, S, W, L>;
            }
        case 0b11:
            break;
        default:
            break;
    }
    return &ARM7TDMI::ARMUnimplemented;
}

template<u16 instruction>
static constexpr THUMBInstructionPtr GetTHUMBInstruction() {
    return &ARM7TDMI::THUMBUnimplemented;
}

static constexpr ARMInstructionPtr* _BuildARMTable(ARMInstructionPtr lut[ARMInstructionTableSize]) {
    static_for<size_t, 0, ARMInstructionTableSize>(
            [&](auto i) {
                lut[i] = GetARMInstruction<i>();
            }
    );
    return lut;
}

static constexpr THUMBInstructionPtr* _BuildTHUMBTable(THUMBInstructionPtr lut[THUMBInstructionTableSize]) {
    static_for<size_t, 0, THUMBInstructionTableSize>(
            [&](auto i) {
                lut[i] = GetTHUMBInstruction<i>();
            }
    );
    return lut;
}

void ARM7TDMI::BuildARMTable() {
    ::_BuildARMTable(ARMInstructions);
}

void ARM7TDMI::BuildTHUMBTable() {
    ::_BuildTHUMBTable(THUMBInstructions);
}