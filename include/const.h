#ifndef GC__CONST_H
#define GC__CONST_H

#define BIOS_FILE "D:/Data/CProjects/DSHBA/files/gba_bios.bin"

#define CLOCK_FREQUENCY 16853760
// from Tonc:
#define CYCLES_PER_FRAME 280896
#define CYCLES_HDRAW 960
#define CYCLES_HBLANK 272
#define CYCLES_HBLANK_FLAG_DELAY 46
#define CYCLES_PER_SCANLINE 1232

// also defined in GX_Helpers.h, but since the PPU can be unlinked now, these need to be def'ed somewhere else
#define VISIBLE_SCREEN_HEIGHT 160u
#define VISIBLE_SCREEN_WIDTH 240u
#define TOTAL_SCREEN_HEIGHT 228u

static_assert((CYCLES_HDRAW + CYCLES_HBLANK) == CYCLES_PER_SCANLINE, "Cycle count mismatch");

#endif //GC__CONST_H
