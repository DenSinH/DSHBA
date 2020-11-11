#include <cstdio>
#include <cstdlib>
#include <cstdint>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

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


enum class CPSRFlags : u32 {
    N    = 0x8000'0000,
    Z    = 0x4000'0000,
    C    = 0x2000'0000,
    V    = 0x1000'0000,
    I    = 0x0000'0080,
    F    = 0x0000'0040,
    T    = 0x0000'0020,
    Mode = 0x0000'001f,
};

constexpr bool CheckCondition(const u8 cond, const u32 cpsr) {
    switch (static_cast<Condition>(cond)) {
        case Condition::EQ:
            return (cpsr & static_cast<u32>(CPSRFlags::Z)) != 0;
        case Condition::NE:
            return (cpsr & static_cast<u32>(CPSRFlags::Z)) == 0;
        case Condition::CS:
            return (cpsr & static_cast<u32>(CPSRFlags::C)) != 0;
        case Condition::CC:
            return (cpsr & static_cast<u32>(CPSRFlags::C)) == 0;
        case Condition::MI:
            return (cpsr & static_cast<u32>(CPSRFlags::N)) != 0;
        case Condition::PL:
            return (cpsr & static_cast<u32>(CPSRFlags::N)) == 0;
        case Condition::VS:
            return (cpsr & static_cast<u32>(CPSRFlags::V)) != 0;
        case Condition::VC:
            return (cpsr & static_cast<u32>(CPSRFlags::V)) == 0;
        case Condition::HI:
            return ((cpsr & static_cast<u32>(CPSRFlags::C)) != 0u) && ((cpsr & static_cast<u32>(CPSRFlags::Z)) == 0u);
        case Condition::LS:
            return ((cpsr & static_cast<u32>(CPSRFlags::C)) == 0u) || ((cpsr & static_cast<u32>(CPSRFlags::Z)) != 0u);
        case Condition::GE:
            return ((cpsr & static_cast<u32>(CPSRFlags::V)) != 0) == ((cpsr & static_cast<u32>(CPSRFlags::N)) != 0);
        case Condition::LT:
            return ((cpsr & static_cast<u32>(CPSRFlags::V)) != 0) ^ ((cpsr & static_cast<u32>(CPSRFlags::N)) != 0);
        case Condition::GT:
            return (!(cpsr & static_cast<u32>(CPSRFlags::Z))) &&
                   (((cpsr & static_cast<u32>(CPSRFlags::V)) != 0) == ((cpsr & static_cast<u32>(CPSRFlags::N)) != 0));
        case Condition::LE:
            return ((cpsr & static_cast<u32>(CPSRFlags::Z)) != 0) ||
                   (((cpsr & static_cast<u32>(CPSRFlags::V)) != 0) ^ ((cpsr & static_cast<u32>(CPSRFlags::N)) != 0));
        case Condition::AL:
            return true;
        default:
            // invalid
            return false;
    }
}

int main() {

    u16 table[16] = {};

    for (u8 cond = 0; cond <= 0xf; cond++) {
        for (u32 flags = 0; flags <= 0xf; flags++) {
            if (CheckCondition(cond, flags << 28u)) {
                table[cond] |= 1u << flags;
            }
        }
    }

    for (u8 cond = 0; cond <= 0xf; cond++) {
        printf("/* %x */ 0x%04x,\n", cond, table[cond]);
    }

    return 0;
}
