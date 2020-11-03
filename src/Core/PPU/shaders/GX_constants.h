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
    BG2CNT  = 0x0cu,
    BG0HOFS = 0x10u,
    BG0VOFS = 0x12u,
};

enum class DPSCNTBits : unsigned int {
    DPFrameSelect = 0x0010u,
    OAM2DMap      = 0x0040u,
    DisplayBG0    = 0x0100u,
    DisplayBG1    = 0x0200u,
    DisplayBG2    = 0x0400u,
    DisplayBG3    = 0x0800u,
};

enum class OBJ_ATTR0 : unsigned int {
    ATTR0_Y  = 0x00ffu,

    ATTR0_OM      = 0x0300u,
    ATTR0_REG     = 0x0000u,
    ATTR0_AFF     = 0x0100u,
    ATTR0_HIDE    = 0x0200u,
    ATTR0_AFF_DBL = 0x0300u,

    ATTR0_GM      = 0x0c00u,
    ATTR0_NORMAL  = 0x0000u,
    ATTR0_BLEND   = 0x0400u,
    ATTR0_WIN     = 0x0800u,

    ATTR0_MOSAIC  = 0x1000u,

    ATTR0_CM      = 0x2000u,
    ATTR0_4BPP    = 0x0000u,
    ATTR0_8BPP    = 0x2000u,

    ATTR0_Sh      = 0xc000u,
    ATTR0_SQUARE  = 0x0000u,
    ATTR0_WIDE    = 0x1000u,
    ATTR0_TALL    = 0x2000u,
};

enum class OBJ_ATTR1 : unsigned int {
    ATTR1_X   = 0x01ffu,
    ATTR1_AFF = 0x3e00u,
    ATTR1_HF  = 0x1000u,
    ATTR1_VF  = 0x2000u,
    ATTR1_Sz  = 0xc000u,
};

enum class OBJ_ATTR2 : unsigned int {
    ATTR2_TID = 0x03ffu,
    ATTR2_PR  = 0x0c00u,
    ATTR2_PB  = 0xf000u,
};

#define VISIBLE_SCREEN_HEIGHT 160
#define VISIBLE_SCREEN_WIDTH 240
#define TOTAL_SCREEN_HEIGHT 228

#endif //GC__GX_CONSTANTS_H
