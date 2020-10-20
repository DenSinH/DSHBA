#include "init.h"
#include "src/Frontend/interface.h"

#include "default.h"

#include <memory>

static GBA* gba;

static u8 ReadByte(u64 offset) {
    return gba->memory.Read<u8, false>(offset);
}

static u32 ValidAddresCheck(u32 address) {
    // all memory can be read for the GBA
    return 1;
}

GBA* init() {
    gba = (GBA*)malloc(sizeof(GBA));
    new(gba) GBA;

    frontend_init(
            &gba->shutdown,
            nullptr,
            nullptr,
            0,
            ValidAddresCheck,
            ReadByte,
            nullptr
    );

    return gba;
}
