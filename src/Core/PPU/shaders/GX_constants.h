#ifndef GC__GX_CONSTANTS_H
#define GC__GX_CONSTANTS_H

enum class BufferBindings : unsigned int {
    PALUBO = 3,
    OAMUBO = 4,
    IOUBO = 5,
    VRAMSSBO = 6,
};

enum class VMEMSizes : unsigned int {
    PALSize = 0x400,
    OAMSize = 0x400,
    VRAMSize = 0x18000,
    // todo: only display IO
    IOSize = 0x400
};

#define VISIBLE_SCREEN_HEIGHT 160
#define VISIBLE_SCREEN_WIDTH 240
#define TOTAL_SCREEN_HEIGHT 228

#endif //GC__GX_CONSTANTS_H
