#pragma once

#include "default.h"

#include "../Mem/Mem.h"

class GBA;

#define sp registers[13]
#define lr registers[14]
#define pc registers[15]

typedef struct s_CPSR {
    unsigned N: 1;
    unsigned Z: 1;
    unsigned C: 1;
    unsigned V: 1;
    unsigned: 20;  // reserved
    unsigned I: 1;
    unsigned F: 1;
    unsigned T: 1;
    u8 mode: 5;
} s_CPSR;


class ARM7TDMI {
    public:
        ARM7TDMI(Mem* memory) {
            this->memory = memory;
            this->pc = 0x0800'0000;
        }
        ~ARM7TDMI() {

        };

        void SkipBIOS() {
            registers[0] = 0x0800'0000;
            registers[1] = 0xEA;
        }
        void Step();

    private:
        friend GBA* init();

        enum class State : u8 {
            ARM   = 0,
            THUMB = 1,
        };

        enum class Mode : u8 {
            User        = 0b10000,
            FIQ         = 0b10001,
            IRQ         = 0b10010,
            Supervisor  = 0b10011,
            Abort       = 0b10111,
            Undefined   = 0b11011,
            System      = 0b11111,
        };

        s_CPSR CPSR = {};
        s_CPSR SPSR = {};
        u32 registers[16] = {};

        Mem* memory;
};
