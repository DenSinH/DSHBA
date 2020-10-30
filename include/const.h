#ifndef GC__CONST_H
#define GC__CONST_H

#define BIOS_FILE "D:/Data/CProjects/DSHBA/files/gba_bios.bin"

// from Tonc:
#define CYCLES_PER_FRAME 280896
#define CYCLES_HBLANK_CLEAR 1006
#define CYCLES_HBLANK_SET 226
#define CYCLES_PER_SCANLINE 1232

static_assert((CYCLES_HBLANK_CLEAR + CYCLES_HBLANK_SET) == CYCLES_PER_SCANLINE, "Cycle count mismatch");

#endif //GC__CONST_H
