#ifndef GC__GX_CONSTANTS_H
#define GC__GX_CONSTANTS_H

enum class BufferBindings : unsigned int {
    PAL      = 1,
    OAM      = 2,
    LCDIO    = 3,
    VRAMSSBO = 4,
};

enum class VMEMSizes : unsigned int {
    PALSize = 0x400u,
    OAMSize = 0x400u,
    VRAMSize = 0x18000u,
    // todo: only display MMIO
    IOSize = 0x54u
};

enum class LCDIORegs : unsigned int {
    DISPCNT = 0x00u,
    BG0CNT  = 0x08u,
    BG0HOFS = 0x10u,
    BG0VOFS = 0x12u,
};

enum class DPSCNTBits : unsigned int {
    DPFrameSelect = 0x0010u,
    DisplayBG0    = 0x0100u,
    DisplayBG1    = 0x0200u,
    DisplayBG2    = 0x0400u,
    DisplayBG3    = 0x0800u,
};

#define VISIBLE_SCREEN_HEIGHT 160
#define VISIBLE_SCREEN_WIDTH 240
#define TOTAL_SCREEN_HEIGHT 228

#endif //GC__GX_CONSTANTS_H
