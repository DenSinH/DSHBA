#include <stdint.h>

typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

u32* IRQCheckFlagsPtr  = (u32*)0x03007ff8;  // DAT_0800c88c
u16* IFPtr             = (u16*)0x04000202;  // DAT_0800c890
u16* IEPtr             = (u16*)0x04000202;
u16* IMEPtr            = (u16*)0x04000208;
u32* TM0CNT            = (u32*)0x04000100;  // DAT_0800c910
const u32 TMCNTSetting =       0x00c0f000;  // DAT_0800c914

void FUN_0800f150(void (*waitloop)(int), void (*on_interrupt)(u16, u16), u16 loop_counter, u16 param_4) {
    // params in r0, r1, r2, r3 on entry
    u32 local_14 = loop_counter;  // param_3
    u32 local_10 = param_4;

    // copy waitloop into stack
    // lr = pointer to LAB0800f178 (THUMB), which resets the stack and returns as return value for the interrupt
    // jump to wait loop
    waitloop(loop_counter);
    on_interrupt(loop_counter, param_4);
}

// waitloop function:
void LAB_0800c2fc(int count) {
    for (; count != 0; count--) {}
}

// function run on interrupt
void FUN_0800c308(u16 loop_counter, u16 param_2)  {
    // loop_counter : r0
    // param_2      : r1
    // pretty big function
}

void FUN_0800c8c8(int timer) {
    *(TM0CNT + timer * 4) = 0;  // clear TMxCNT (stride 4)
    *(TM0CNT + timer * 4) = TMCNTSetting;
    FUN_0800f150(LAB_0800c2fc, FUN_0800c308, 0x40a, 0);
    *(TM0CNT + timer * 4) = 0;  // clear TMxCNT again
}

// Timer IRQ test
u32 sub_0800c7b4() {
    u32 failed = 0;  // local_10

    for (int i = 0; i < 4; i++) {
        /*  TEST INTERRUPT ON OVERFLOW */
        // clear test and enable interrupts
        *IRQCheckFlagsPtr = 0;
        *IFPtr  = (u16)(8 << i);
        *IEPtr  = (u16)(8 << i);  // FUN_0800d760, stores old IE value
        *IMEPtr = 1;              // FUN_0800d730, stores old IME value

        FUN_0800c8c8(i);
        *IMEPtr = 0;              // FUN_0800d730, stores old IME value
        if (((*IRQCheckFlagsPtr) & 8 << i) == 0) {
            failed |= 1 << (i << 1);
        }

        /*  TEST NO INTERRUPT ON OVERFLOW IF DISABLED IN IE*/
        *IRQCheckFlagsPtr = 0;
        *IFPtr  = (u16)(8 << i);
        *IEPtr  = 0;              // FUN_0800d760, stores old IE value
        *IMEPtr = 1;              // FUN_0800d730, stores old IME value

        FUN_0800c8c8(i);
        *IMEPtr = 0;              // FUN_0800d730, stores old IME value
        if (((*IRQCheckFlagsPtr) & 8 << i) != 0) {
            failed |= 2 << (i << 1);
        }
    }

    // failed in r0 and r1 at 0800c89e
    return failed;
}
