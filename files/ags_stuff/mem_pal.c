/*
 * This is only the test with flag 0x10
 * */

unsigned SetIME(unsigned new_val);
unsigned div(unsigned, unsigned);  // FUN_080125c0 (I think this does division)

unsigned* TM0CNT = 0x04000100;     // DAT_0800'd180
unsigned* DMA3CNT_L = 0x04000dc;   // DAT_0800'd184
unsigned** DMA3SAD = 0x04000d4;    // DAT_0800'd188
unsigned** DMA3DAD = 0x04000d8;    // DAT_0800'd18c
unsigned DMASetting = 0x85000040;  // DAT_0800'd190
unsigned DMASetting2 = 0x84000040;  // DAT_0800'd1e4

typedef char bool;

bool FUN_0800d118(unsigned* src, short p2, unsigned p3) {
    // src = (unsigned*)0x0500'0000  // PAL start
    // p2 = 0
    // p3 = 0x10

    unsigned IME = SetIME(0);  // FUN_0800d730
    *TM0CNT = 0;
    *TM0CNT = 0x00800000;  // enable, reload 0

    *DMA3CNT_L = 0;
    *DMA3SAD = TM0CNT;
    *DMA3DAD = src;
    /*
     * Count: 0x40
     * Setting: 0x8500 (enable, 32 bit, fixed source)
     * */
    *DMA3CNT_L = DMASetting;
    do {} while((*DMA3CNT_L) & 0x8000000);  // wait for DMA completion

    *DMA3CNT_L = 0;
    *DMA3SAD = src;
    *DMA3DAD = 0xabcdef;       // local_120, some location in WRAM (not this value)
    *DMA3CNT_L = DMASetting2;  // (enable, 32 bit)

    do {} while((*DMA3CNT_L) & 0x8000000);  // wait for DMA completion

    short local_120[129];  // array, probably holds results
    local_120[128] = div(0x20, p3) * (p2 + 1) + 1;  // so this would be 3 then

    short tmval     = local_120[0];     // local_1e
    short tmsetting = *(TM0CNT + 2);    // local_1c, read TM0CNT_L
    short counter = 0;
    bool error = 0;

    unsigned flags;
    for (unsigned counter = 0; counter < 0x7f; counter += 2) {
        if (local_120[counter] != tmsetting) {
            error = 1;
            break;
        }

        if (local_120[counter + 1] != tmval) {
            error = 1;
            break;
        }

        tmval += local_120[128];
    }
    return error;
}