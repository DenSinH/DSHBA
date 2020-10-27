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