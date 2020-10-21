#include "ARM7TDMI.h"
#include "CoreUtils.h"

template<u32 instruction>
static constexpr ARMInstructionPtr GetARMInstruction() {
    switch ((instruction & 0xc00) >> 10) {
        case 0b00:
        {
            if constexpr (instruction == 0b000100100001) {
                // BX
                break;
            }
            else if constexpr((instruction & 0b1001) != 0b1001) {
                const bool I = (instruction & ARM7TDMI::ARMHash(0x0200'0000)) != 0;
                const u8 opcode = (instruction >> 5) & 0xf;
                const bool S = (instruction & ARM7TDMI::ARMHash(0x0010'0000)) != 0;
                const u8 shift_type = (instruction & 6) >> 1;
                // Data Processing or PSR transfer

                if constexpr((instruction & ARM7TDMI::ARMHash(0x0010'0000)) && (0b1000 <= opcode) && (opcode <= 0b1011)) {
                    // PSR transfer
                    break;
                }
                else {
                    return &ARM7TDMI::DataProcessing<I, opcode, S, shift_type>;
                }
            }
            else {
                break;
            }
        }
        case 0b01:
            break;
        case 0b10:
            if constexpr((instruction & (ARM7TDMI::ARMHash(0x0e00'0000))) == ARM7TDMI::ARMHash(0x0a00'0000)) {
                return &ARM7TDMI::Branch<(instruction & ARM7TDMI::ARMHash(0x0100'0000)) != 0>;
            }
            break;
        case 0b11:
            break;
        default:
            break;
    }
    return &ARM7TDMI::ARMUnimplemented;
}

template<u32 instruction>
static constexpr THUMBInstructionPtr GetTHUMBInstruction() {
    return &ARM7TDMI::THUMBUnimplemented;
}

constexpr ARMInstructionPtr* _BuildARMTable(ARMInstructionPtr lut[ARMInstructionTableSize]) {
    static_for<size_t, 0, ARMInstructionTableSize>(
            [&](auto i) {
                lut[i] = GetARMInstruction<i>();
            }
    );
    return lut;
}

constexpr THUMBInstructionPtr* _BuildTHUMBTable(THUMBInstructionPtr lut[THUMBInstructionTableSize]) {
//    for (size_t i = 0; i < THUMBInstructionTableSize; i++) {
//        this->THUMBInstructions[i] = &ARM7TDMI::THUMBUnimplemented;
//    }
    return lut;
}

void ARM7TDMI::BuildARMTable() {
    ::_BuildARMTable(ARMInstructions);
}

void ARM7TDMI::BuildTHUMBTable() {
    ::_BuildTHUMBTable(THUMBInstructions);
}