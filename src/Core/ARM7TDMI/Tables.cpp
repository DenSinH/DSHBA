#include "ARM7TDMI.h"
#include "CoreUtils.h"

template<u32 instruction>
static constexpr ARMInstructionPtr GetARMInstruction() {
    switch ((instruction & 0xc00) >> 10) {
        case 0b00:
        {
            if constexpr((instruction & ARMHash(0x0fc0'00f0)) == ARMHash(0x0000'0090)) {
                // multiply
                const bool A = (instruction & ARMHash(0x0020'0000)) != 0;
                const bool S = (instruction & ARMHash(0x0010'0000)) != 0;
                return &ARM7TDMI::Multiply<A, S>;
            }
            else if constexpr((instruction & ARMHash(0x0f80'00f0)) == ARMHash(0x0080'0090)) {
                // multiply long
                const bool U = (instruction & ARMHash(0x0040'0000)) != 0;
                const bool A = (instruction & ARMHash(0x0020'0000)) != 0;
                const bool S = (instruction & ARMHash(0x0010'0000)) != 0;
                return &ARM7TDMI::MultiplyLong<U, A, S>;
            }
            else if constexpr((instruction & ARMHash(0x0fb0'0ff0)) == ARMHash(0x0100'0090)) {
                // SWP
                const bool B = (instruction & ARMHash(0x0040'0000)) != 0;
                return &ARM7TDMI::SWP<B>;
            }
            else if constexpr(instruction == 0b0001'0010'0001) {
                // BX
                static_assert(ARM7TDMI::IsARMBranch[instruction], "ARM bx missed");
                return &ARM7TDMI::BranchExchange;
            }
            else if constexpr((instruction & ARMHash(0x0e00'0090)) == ARMHash(0x0000'0090)) {
                // Halfword Data Transfer Imm/Reg Offset
                const bool P = (instruction & ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARMHash(0x0080'0000)) != 0;
                const bool imm_offset = (instruction & ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARMHash(0x0010'0000)) != 0;
                const bool S = (instruction & ARMHash(0x0000'0040)) != 0;
                const bool H = (instruction & ARMHash(0x0000'0020)) != 0;
                return &ARM7TDMI::HalfwordDataTransfer<P, U, imm_offset, W, L, S, H>;
            }
            else {
                // Data Processing or PSR transfer
                const u8 opcode = (instruction >> 5) & 0xf;
                const bool S = (instruction & ARMHash(0x0010'0000)) != 0;

                if constexpr(!S && (0b1000 <= opcode) && (opcode <= 0b1011)) {
                    // PSR transfer
                    const bool P = (instruction & ARMHash(0x0040'0000)) != 0;
                    if constexpr((instruction & (ARMHash(0x0fb0'00f0))) == ARMHash(0x0100'0000)) {
                        // MRS (PSR -> regs)
                        return &ARM7TDMI::MRS<P>;
                    }
                    else {
                        // MSR (reg/imm -> PSR)
                        const bool I = (instruction & ARMHash(0x0200'0000)) != 0;
                        return &ARM7TDMI::MSR<I, P>;
                    }
                }
                else {
                    const bool I = (instruction & ARMHash(0x0200'0000)) != 0;
                    const u8 shift_type = (instruction & 6) >> 1;
                    // has to be == 0, I messed this up, but this is the way it is now...
                    // doesn't make much of a difference in the end, its nicer to use true for immediate shift anyway,
                    // cause I need to use it in ldr/str anyway
                    const bool shift_imm = (instruction & ARMHash(0x0000'0010)) == 0;

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
            if constexpr((instruction & (ARMHash(0x0e00'0010))) == ARMHash(0x0600'0010)) {
                // undefined
                return &ARM7TDMI::ARMUnimplemented;
            }
            else {
                // Single Data Transfer
                const bool I = (instruction & ARMHash(0x0200'0000)) != 0;
                const bool P = (instruction & ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARMHash(0x0080'0000)) != 0;
                const bool B = (instruction & ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARMHash(0x0010'0000)) != 0;
                const u8 shift_type = (instruction & 6) >> 1;

                return &ARM7TDMI::SingleDataTransfer<I, P, U, B, W, L, shift_type>;
            }
        case 0b10:
            if constexpr((instruction & (ARMHash(0x0e00'0000))) == ARMHash(0x0a00'0000)) {
                // Branch (with Link)
                static_assert(ARM7TDMI::IsARMBranch[instruction], "ARM branch missed");
                return &ARM7TDMI::Branch<(instruction & ARMHash(0x0100'0000)) != 0>;
            }
            else {
                // Block Data Transfer
                const bool P = (instruction & ARMHash(0x0100'0000)) != 0;
                const bool U = (instruction & ARMHash(0x0080'0000)) != 0;
                const bool S = (instruction & ARMHash(0x0040'0000)) != 0;
                const bool W = (instruction & ARMHash(0x0020'0000)) != 0;
                const bool L = (instruction & ARMHash(0x0010'0000)) != 0;
                return &ARM7TDMI::BlockDataTransfer<P, U, S, W, L>;
            }
        case 0b11:
            if constexpr((instruction & (ARMHash(0x0f00'0000))) == ARMHash(0x0f00'0000)) {
                static_assert(ARM7TDMI::IsARMBranch[instruction], "ARM swi missed");
                return &ARM7TDMI::SWI<u32>;
            }
            // actually should also hold coprocessor stuff, but that doesnt exist anyway
            break;
        default:
            break;
    }
    return &ARM7TDMI::ARMUnimplemented;
}

template<u16 instruction>
static constexpr THUMBInstructionPtr GetTHUMBInstruction() {
    if constexpr((instruction & THUMBHash(0xe000)) == THUMBHash(0x0000)) {
        if constexpr((instruction & THUMBHash(0xf800)) != THUMBHash(0x1800)) {
            const u8 Op = instruction >> 5;
            const u8 Offs5 = instruction & 0x1f;
            return &ARM7TDMI::MoveShifted<Op, Offs5>;
        }
        else {
            const bool I  = (instruction & THUMBHash(0x0400)) != 0;
            const bool Op = (instruction & THUMBHash(0x0200)) != 0;
            const u8 RnOff3 = (instruction & 0x7);
            return &ARM7TDMI::AddSubtract<I, Op, RnOff3>;
        }
    }
    else if constexpr((instruction & THUMBHash(0xe000)) == THUMBHash(0x2000)) {
        const u8 Op = (instruction >> 5) & 0x3;
        const u8 rd = (instruction >> 2) & 0x7;
        return &ARM7TDMI::ALUImmediate<Op, rd>;
    }
    else if constexpr((instruction & THUMBHash(0xfc00)) == THUMBHash(0x4000)) {
        const u8 opcode = (instruction & 0xf);
        return &ARM7TDMI::ALUOperations<opcode>;
    }
    else if constexpr((instruction & THUMBHash(0xfc00)) == THUMBHash(0x4400)) {
        const u8 opcode = (instruction & 0xc) >> 2;
        const bool H1 = (instruction & THUMBHash(0x0080)) != 0;
        const bool H2 = (instruction & THUMBHash(0x0040)) != 0;
        if constexpr(opcode == 3) {
            static_assert(ARM7TDMI::IsTHUMBBranch[instruction], "THUMB bx missed");
        }
        return &ARM7TDMI::HiRegOps_BX<opcode, H1, H2>;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0x8000)) {
        const bool L   = (instruction & THUMBHash(0x0800)) != 0;
        const u8 Offs5 = (instruction & 0x1f);
        return &ARM7TDMI::LoadStoreHalfword<L, Offs5>;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0x5000)) {
        const bool H_L = (instruction & THUMBHash(0x0800)) != 0;
        const bool S_B = (instruction & THUMBHash(0x0400)) != 0;
        const u8 ro  = (instruction & 0x7);
        if (instruction & THUMBHash(0x0200)) {
            return &ARM7TDMI::LoadStoreSEBH<H_L, S_B, ro>;
        }
        else {
            return &ARM7TDMI::LoadStoreRegOffs<H_L, S_B, ro>;
        }
    }
    else if constexpr((instruction & THUMBHash(0xe000)) == THUMBHash(0x6000)) {
        const bool B = (instruction & THUMBHash(0x1000)) != 0;
        const bool L = (instruction & THUMBHash(0x0800)) != 0;
        const u8 Offs5 = (instruction & 0x1f);
        return &ARM7TDMI::LoadStoreImmOffs<B, L, Offs5>;
    }
    else if constexpr((instruction & THUMBHash(0xf600)) == THUMBHash(0xb400)) {
        const bool L = (instruction & THUMBHash(0x0800)) != 0;
        const bool R = (instruction & THUMBHash(0x0100)) != 0;
        return &ARM7TDMI::PushPop<L, R>;
    }
    else if constexpr((instruction & THUMBHash(0xf800)) == THUMBHash(0x4800)) {
        const u8 rd = (instruction >> 2) & 7;
        return &ARM7TDMI::PCRelativeLoad<rd>;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0xd000)) {
        if constexpr((instruction & THUMBHash(0xff00)) == THUMBHash(0xdf00)) {
            // SWI
            static_assert(ARM7TDMI::IsTHUMBBranch[instruction], "THUMB swi missed");
            return &ARM7TDMI::SWI<u16>;
        }
        else {
            const u8 cond = (instruction >> 2) & 0xf;
            static_assert(ARM7TDMI::IsTHUMBBranch[instruction], "THUMB conditional branch missed");
            return &ARM7TDMI::ConditionalBranch<cond>;
        }
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0xf000)) {
        const bool H = (instruction & THUMBHash(0x0800)) != 0;
        static_assert(ARM7TDMI::IsTHUMBBranch[instruction], "THUMB long branch missed");
        return &ARM7TDMI::LongBranchLink<H>;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0xc000)) {
        const bool L = (instruction & THUMBHash(0x0800)) != 0;
        const u8 rb = (instruction >> 2) & 7;
        return &ARM7TDMI::MultipleLoadStore<L, rb>;
    }
    else if constexpr((instruction & THUMBHash(0xf800)) == THUMBHash(0xe000)) {
        static_assert(ARM7TDMI::IsTHUMBBranch[instruction], "THUMB unconditional branch missed");
        return &ARM7TDMI::UnconditionalBranch;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0xa000)) {
        const bool SP = (instruction & THUMBHash(0x0800)) != 0;
        const u8 rd = (instruction >> 2) & 7;
        return &ARM7TDMI::LoadAddress<SP, rd>;
    }
    else if constexpr((instruction & THUMBHash(0xff00)) == THUMBHash(0xb000)) {
        const bool S = (instruction & THUMBHash(0x0080)) != 0;
        return &ARM7TDMI::AddOffsToSP<S>;
    }
    else if constexpr((instruction & THUMBHash(0xf000)) == THUMBHash(0x9000)) {
        const bool L = (instruction & THUMBHash(0x0800)) != 0;
        const u8 rd = (instruction >> 2) & 7;
        return &ARM7TDMI::SPRelativeLoadStore<L, rd>;
    }

    return &ARM7TDMI::THUMBUnimplemented;
}

static constexpr ARMInstructionPtr* staticBuildARMTable(ARMInstructionPtr lut[ARMInstructionTableSize]) {
    static_for<size_t, 0, ARMInstructionTableSize>(
            [&](auto i) {
                lut[i] = GetARMInstruction<i>();
            }
    );
    return lut;
}

static constexpr THUMBInstructionPtr* staticBuildTHUMBTable(THUMBInstructionPtr lut[THUMBInstructionTableSize]) {
    static_for<size_t, 0, THUMBInstructionTableSize>(
            [&](auto i) {
                lut[i] = GetTHUMBInstruction<i>();
            }
    );
    return lut;
}

void ARM7TDMI::BuildARMTable() {
    ::staticBuildARMTable(ARMInstructions);
}

void ARM7TDMI::BuildTHUMBTable() {
    ::staticBuildTHUMBTable(THUMBInstructions);
}