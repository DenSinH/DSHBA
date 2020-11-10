#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

void _memset(u32 addr, u32 len, u32 val, u32 flags /* size ? */);

u32* DMA0SAD = (u32*)0x040000b0;
u32* DMA0DAD = (u32*)0x040000b4;
u32* DMA0CNT = (u32*)0x040000b8;

u32* DISPSTATPtr = (u32*)0x04000004;

// DMA HBlank test
u32 sub_0800a488() {
    u32 DISPSTAT      = *DISPSTATPtr;
    u32 DMACNTSetting = 0xA3000001;        // local_18
    u32 dest          = 0x02000000;        // local_1c
    u16* VCountPtr    = (u16*)0x04000006;  // local_20
    (*DISPSTATPtr)    = 0x10;              // enable HBlank IRQ
    u32 failed        = 0;                 // local_28

    for (int DMAChannel = 0; /* local_14 */ DMAChannel < 4; DMAChannel++) {
        _memset(dest, 0x1c8, 0, 2);
        do { } while ((*VCountPtr) != 0xe3);  // wait until VBlank
        do { } while ((*VCountPtr) == 0xe3);  // wait one more cycle to be sure the DMA happened

        *(DMA0SAD + 0xc * DMAChannel) = VCountPtr;  // ??
        *(DMA0DAD + 0xc * DMAChannel) = dest;
        *(DMA0CNT + 0xc * DMAChannel) = DMACNTSetting;

        for (int i = 0; /* local_10 */ i < 0xe4; i++) {
            // wait for HBlank IRQ
        }

        // check if all data was copied
        *(DMA0CNT + 0xc * DMAChannel) = 0;  // turn off DMA
        u16* dest_check = (u16*)dest;
        for (int i = 0; /* local_10 */ i < 0xa0; i++) {
            u16 val = *dest_check;
            dest_check += 1;  // +1 means advance 2 bytes, stride of 2
            if (val != i) {
                // @800a5a6
                failed |= 1 << DMAChannel;
            }
        }

        // check if no more data was copied
        for (int i = 0; /* local_10 */ i < 0xe4; i++) {
            u16 val = *dest_check;
            dest_check += 1;
            if (val != 0) {
                // @800a5d2
                failed |= 1 << DMAChannel;
            }
        }

    }

    // reset DISPSTAT state
    *DISPSTATPtr = DISPSTAT;

    // FUN_0800d854
    // failed in r1 and r0 at 0800cb42
    return failed;
}
