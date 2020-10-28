#pragma once

#include "default.h"

enum class Interrupt : u16 {
    VBlank = 0x0001,
    HBlank = 0x0002,
    VCount = 0x0004,
    Timer0 = 0x0008,
    Timer1 = 0x0010,
    Timer2 = 0x0020,
    Timer3 = 0x0040,
    SIO    = 0x0080,
    DMA0   = 0x0100,
    DMA1   = 0x0200,
    DMA2   = 0x0400,
    DMA3   = 0x0800,
    Keypad = 0x1000,
    Extern = 0x2000,
};
