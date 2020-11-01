#pragma once

#include "default.h"

enum class DISPSTATFlags : u16 {
    VBlank        = 0x0001,
    HBLank        = 0x0002,
    VCount        = 0x0004,
    VBlankIRQ     = 0x0008,
    HBLankIRQ     = 0x0010,
    VCountIRQ     = 0x0020,
};

enum class DMACNT_HFlags : u16 {
    DestAddrControl     = 0x0060,
    DestIncrement       = 0x0000,
    DestDecrement       = 0x0020,
    DestFixed           = 0x0040,
    DestIncrementReload = 0x0060,

    SrcAddrControl      = 0x0180,
    SrcIncrement        = 0x0000,
    SrcDecrement        = 0x0080,
    SrcFixed            = 0x0100,

    Repeat              = 0x0200,
    WordSized           = 0x0400,

    StartTiming         = 0x3000,
    StartImmediate      = 0x0000,
    StartVBlank         = 0x1000,
    StartHBlank         = 0x2000,
    StartSpecial        = 0x3000,

    IRQ                 = 0x4000,
    Enable              = 0x8000,
};

enum class TMCNT_HFlags : u16 {
    Prescaler = 0x0003,
    CountUp   = 0x0004,
    IRQ       = 0x0040,
    Enabled   = 0x0080,
};