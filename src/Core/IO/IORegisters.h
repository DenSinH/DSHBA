#pragma once

#include "default.h"

enum class IORegister : u32 {
    DISPCNT     = 0x000,  //  2    R/W   LCD Control
    //          = 0x002,  //  2    R/W   Undocumented - Green Swap
    DISPSTAT    = 0x004,  //  2    R/W   General LCD Status (STAT,LYC)
    VCOUNT      = 0x006,  //  2    R     Vertical Counter (LY)
    BG0CNT      = 0x008,  //  2    R/W   BG0 Control
    BG1CNT      = 0x00A,  //  2    R/W   BG1 Control
    BG2CNT      = 0x00C,  //  2    R/W   BG2 Control
    BG3CNT      = 0x00E,  //  2    R/W   BG3 Control
    BG0HOFS     = 0x010,  //  2    W     BG0 X-Offset
    BG0VOFS     = 0x012,  //  2    W     BG0 Y-Offset
    BG1HOFS     = 0x014,  //  2    W     BG1 X-Offset
    BG1VOFS     = 0x016,  //  2    W     BG1 Y-Offset
    BG2HOFS     = 0x018,  //  2    W     BG2 X-Offset
    BG2VOFS     = 0x01A,  //  2    W     BG2 Y-Offset
    BG3HOFS     = 0x01C,  //  2    W     BG3 X-Offset
    BG3VOFS     = 0x01E,  //  2    W     BG3 Y-Offset
    BG2PA       = 0x020,  //  2    W     BG2 Rotation/Scaling Parameter A (dx)
    BG2PB       = 0x022,  //  2    W     BG2 Rotation/Scaling Parameter B (dmx)
    BG2PC       = 0x024,  //  2    W     BG2 Rotation/Scaling Parameter C (dy)
    BG2PD       = 0x026,  //  2    W     BG2 Rotation/Scaling Parameter D (dmy)
    BG2X        = 0x028,  //  4    W     BG2 Reference Point X-Coordinate
    BG2Y        = 0x02C,  //  4    W     BG2 Reference Point Y-Coordinate
    BG3PA       = 0x030,  //  2    W     BG3 Rotation/Scaling Parameter A (dx)
    BG3PB       = 0x032,  //  2    W     BG3 Rotation/Scaling Parameter B (dmx)
    BG3PC       = 0x034,  //  2    W     BG3 Rotation/Scaling Parameter C (dy)
    BG3PD       = 0x036,  //  2    W     BG3 Rotation/Scaling Parameter D (dmy)
    BG3X        = 0x038,  //  4    W     BG3 Reference Point X-Coordinate
    BG3Y        = 0x03C,  //  4    W     BG3 Reference Point Y-Coordinate
    WIN0H       = 0x040,  //  2    W     Window 0 Horizontal Dimensions
    WIN1H       = 0x042,  //  2    W     Window 1 Horizontal Dimensions
    WIN0V       = 0x044,  //  2    W     Window 0 Vertical Dimensions
    WIN1V       = 0x046,  //  2    W     Window 1 Vertical Dimensions
    WININ       = 0x048,  //  2    R/W   Inside of Window 0 and 1
    WINOUT      = 0x04A,  //  2    R/W   Inside of OBJ Window & Outside of Windows
    MOSAIC      = 0x04C,  //  2    W     Mosaic Size
    //          = 0x04E,  //       -     Not used
    BLDCNT      = 0x050,  //  2    R/W   Color Special Effects Selection
    BLDALPHA    = 0x052,  //  2    R/W   Alpha Blending Coefficients
    BLDY        = 0x054,  //  2    W     Brightness (Fade-In/Out) Coefficient
    //          = 0x056,  //       -     Not used
    /* Sound Registers */
    SOUND1CNT_L = 0x060,  //  2    R/W   Channel 1 Sweep register       (NR10)
    SOUND1CNT_H = 0x062,  //  2    R/W   Channel 1 Duty/Length/Envelope (NR11, NR12)
    SOUND1CNT_X = 0x064,  //  2    R/W   Channel 1 Frequency/Control    (NR13, NR14)
    //          = 0x066,  //       -     Not used
    SOUND2CNT_L = 0x068,  //  2    R/W   Channel 2 Duty/Length/Envelope (NR21, NR22)
    //          = 0x06A,  //       -     Not used
    SOUND2CNT_H = 0x06C,  //  2    R/W   Channel 2 Frequency/Control    (NR23, NR24)
    //          = 0x06E,  //       -     Not used
    SOUND3CNT_L = 0x070,  //  2    R/W   Channel 3 Stop/Wave RAM select (NR30)
    SOUND3CNT_H = 0x072,  //  2    R/W   Channel 3 Length/Volume        (NR31, NR32)
    SOUND3CNT_X = 0x074,  //  2    R/W   Channel 3 Frequency/Control    (NR33, NR34)
    //          = 0x076,  //       -     Not used
    SOUND4CNT_L = 0x078,  //  2    R/W   Channel 4 Length/Envelope      (NR41, NR42)
    //          = 0x07A,  //       -     Not used
    SOUND4CNT_H = 0x07C,  //  2    R/W   Channel 4 Frequency/Control    (NR43, NR44)
    //          = 0x07E,  //       -     Not used
    SOUNDCNT_L  = 0x080,  //  2    R/W   Control Stereo/Volume/Enable   (NR50, NR51)
    SOUNDCNT_H  = 0x082,  //  2    R/W   Control Mixing/DMA Control
    SOUNDCNT_X  = 0x084,  //  2    R/W   Control Sound on/off           (NR52)
    //          = 0x086,  //       -     Not used
    SOUNDBIAS   = 0x088,  //  2    BIOS  Sound PWM Control
    //          = 0x08A,  //  ..     -   Not used
    WAVE_RAM    = 0x090,  // 2x10h R/    Channel 3 Wave Pattern RAM (2 banks!!)
    FIFO_A      = 0x0A0,  //  4    W     Channel A FIFO, Data 0-3
    FIFO_B      = 0x0A4,  //  4    W     Channel B FIFO, Data 0-3
    //          = 0x0A8,  //       -     Not used
    /* DMA Transfer Channels */
    DMA0SAD     = 0x0B0,  //  4    W     DMA 0 Source Address
    DMA0DAD     = 0x0B4,  //  4    W     DMA 0 Destination Address
    DMA0CNT_L   = 0x0B8,  //  2    W     DMA 0 Word Count
    DMA0CNT_H   = 0x0BA,  //  2    R/W   DMA 0 Control
    DMA1SAD     = 0x0BC,  //  4    W     DMA 1 Source Address
    DMA1DAD     = 0x0C0,  //  4    W     DMA 1 Destination Address
    DMA1CNT_L   = 0x0C4,  //  2    W     DMA 1 Word Count
    DMA1CNT_H   = 0x0C6,  //  2    R/W   DMA 1 Control
    DMA2SAD     = 0x0C8,  //  4    W     DMA 2 Source Address
    DMA2DAD     = 0x0CC,  //  4    W     DMA 2 Destination Address
    DMA2CNT_L   = 0x0D0,  //  2    W     DMA 2 Word Count
    DMA2CNT_H   = 0x0D2,  //  2    R/W   DMA 2 Control
    DMA3SAD     = 0x0D4,  //  4    W     DMA 3 Source Address
    DMA3DAD     = 0x0D8,  //  4    W     DMA 3 Destination Address
    DMA3CNT_L   = 0x0DC,  //  2    W     DMA 3 Word Count
    DMA3CNT_H   = 0x0DE,  //  2    R/W   DMA 3 Control
    //          = 0x0E0,  //       -     Not used
    /* Timer Registers */
    TM0CNT_L    = 0x100,  //  2    R/W   Timer 0 Counter/Reload
    TM0CNT_H    = 0x102,  //  2    R/W   Timer 0 Control
    TM1CNT_L    = 0x104,  //  2    R/W   Timer 1 Counter/Reload
    TM1CNT_H    = 0x106,  //  2    R/W   Timer 1 Control
    TM2CNT_L    = 0x108,  //  2    R/W   Timer 2 Counter/Reload
    TM2CNT_H    = 0x10A,  //  2    R/W   Timer 2 Control
    TM3CNT_L    = 0x10C,  //  2    R/W   Timer 3 Counter/Reload
    TM3CNT_H    = 0x10E,  //  2    R/W   Timer 3 Control
    //          = 0x110,  //       -     Not used
    /* Serial Communication (1) */
    SIODATA32   = 0x120,  //  4    R/W   SIO Data (Normal-32bit Mode; shared with below)
    SIOMULTI0   = 0x120,  //  2    R/W   SIO Data 0 (Parent)    (Multi-Player Mode)
    SIOMULTI1   = 0x122,  //  2    R/W   SIO Data 1 (1st Child) (Multi-Player Mode)
    SIOMULTI2   = 0x124,  //  2    R/W   SIO Data 2 (2nd Child) (Multi-Player Mode)
    SIOMULTI3   = 0x126,  //  2    R/W   SIO Data 3 (3rd Child) (Multi-Player Mode)
    SIOCNT      = 0x128,  //  2    R/W   SIO Control Register
    SIOMLT_SEND = 0x12A,  //  2    R/W   SIO Data (Local of MultiPlayer; shared below)
    SIODATA8    = 0x12A,  //  2    R/W   SIO Data (Normal-8bit and UART Mode)
    //          = 0x12C,  //       -     Not used
    /* Keypad Input */
    KEYINPUT    = 0x130,  //  2    R     Key Status
    KEYCNT      = 0x132,  //  2    R/W   Key Interrupt Control
    /* Serial Communication (2) */
    RCNT        = 0x134,  //  2    R/W   SIO Mode Select/General Purpose Data
    IR          = 0x136,  //  -    -     Ancient - Infrared Register (Prototypes only)
    //          = 0x138,  //       -     Not used
    JOYCNT      = 0x140,  //  2    R/W   SIO JOY Bus Control
    //          = 0x142,  //       -     Not used
    JOY_RECV    = 0x150,  //  4    R/W   SIO JOY Bus Receive Data
    JOY_TRANS   = 0x154,  //  4    R/W   SIO JOY Bus Transmit Data
    JOYSTAT     = 0x158,  //  2    R/?   SIO JOY Bus Receive Status
    //          = 0x15A,  //       -     Not used
    /* Interrupt, Waitstate, and Power-Down Control */
    IE          = 0x200,  //  2    R/W   Interrupt Enable Register
    IF          = 0x202,  //  2    R/W   Interrupt Request Flags / IRQ Acknowledge
    WAITCNT     = 0x204,  //  2    R/W   Game Pak Waitstate Control
    //          = 0x206,  //       -     Not used
    IME         = 0x208,  //  2    R/W   Interrupt Master Enable Register
    //          = 0x20A,  //       -     Not used
    POSTFLG     = 0x300,  //  1    R/W   Undocumented - Post Boot Flag
    HALTCNT     = 0x301,  //  1    W     Undocumented - Power Down Control
    //          = 0x302,  //       -     Not used
    //          = 0x410,  //  ?    ?     Undocumented - Purpose Unknown / Bug ??? 0FFh
    //          = 0x411,  //       -     Not used
    //          = 0x800,  //  4    R/W   Undocumented - Internal Memory Control (R/W)
    //          = 0x804,  //       -     Not used
    //          = 0x800,  //  4    R/W   Mirrors of 4000800h (repeated each 64K)
};
