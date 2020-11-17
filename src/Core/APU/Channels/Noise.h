#pragma once

#include "Channel.h"

class Noise : public Channel {

public:
    explicit Noise(s_scheduler* scheduler) : Channel(scheduler) {
        Period = 128 * 100;
    }

    void Trigger() override {
        Channel::Trigger();
        ShiftRegister = CounterStepWidth ? 0x4000 : 0x40;
    }

    bool CounterStepWidth = false;  // false: 15 bit, true: 7 bit

private:
    friend class Initializer;
    u32 ShiftRegister;

protected:

    i16 GetSample() override {
        if ((~ShiftRegister) & 1) {
            return (0x7fff * Volume) >> 4;
        }
        else {
            return (-0x7fff * Volume) >> 4;
        }
    }

    void OnTick() override {
        /*
         * Noise randomly switches between HIGH and LOW levels, the output levels are calculated by a shift register (X),
         * at the selected frequency, as such:
         * 7bit:  X=X SHR 1, IF carry THEN Out=HIGH, X=X XOR 40h ELSE Out=LOW
         * 15bit: X=X SHR 1, IF carry THEN Out=HIGH, X=X XOR 4000h ELSE Out=LOW
         * The initial value when (re-)starting the sound is X=40h (7bit) or X=4000h (15bit).
         * The data stream repeats after 7Fh (7bit) or 7FFFh (15bit) steps.
         * */

        u32 carry = (ShiftRegister ^ (ShiftRegister >> 1)) & 1;
        ShiftRegister >>= 1;
        ShiftRegister |= carry << 15;

        if (carry) {
            if (CounterStepWidth) {
                ShiftRegister ^= 0x6000;  // wrong in GBATek
            }
            else {
                ShiftRegister ^= 0x60;
            }
        }
    }
};