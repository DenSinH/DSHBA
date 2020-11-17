#pragma once

#include "Channel.h"

#include "log.h"

#include <assert.h>

class Wave : public Channel {

public:
    explicit Wave(s_scheduler* scheduler, u8* wave_ram_ptr) : Channel(scheduler) {
        if (!wave_ram_ptr) {
            log_fatal("Invalid pointer to wave ram passed");
        }
        WaveRAM_ptr = wave_ram_ptr;
        Period = 8 * 2048;
    }

    void Trigger() override {
        if (LengthCounter == 0) {
            LengthCounter = 256;  // different max value
        }
        Channel::Trigger();
        PositionCounter = 0;
    }

    void SetVolume(u32 value) {
        /* 0: mute
         * 1: 100%
         * 2: 50%
         * 3: 25%
         * */
        Volume = (0x20 >> value) & 0x1f;
    }

    bool CounterStepWidth = false;  // false: 15 bit, true: 7 bit
    void SwitchBanks(u8 new_bank) {
        if (BankNumber != new_bank) {
            memcpy(Bank, WaveRAM_ptr, 16);     // temp buffer new values
            memcpy(WaveRAM_ptr, WaveRAM, 16);  // copy back old values
            memcpy(WaveRAM, Bank, 16);         // copy over new values
            BankNumber = new_bank;
        }
    }

private:
    friend class MMIO;
    friend class Initializer;

    // whenever the bank switches, we need to copy back the values that are in wave ram for the next bank switch
    // we also copy the values in wave ram to an internal bank to be played later
    u8* WaveRAM_ptr = nullptr;
    u8 Bank[16] = {};
    u8 WaveRAM[16] = {};

    u32 PositionCounter = 0;

    bool DoubleBanked = false;  // false: 1 bank, true: 2 banks
    u8   BankNumber   = 0;
    bool PlayBack     = false;
    bool ForceVolume  = false;

protected:

    [[nodiscard]] ALWAYS_INLINE bool SoundOn() const override {
        if (!Enabled) {
            return false;
        }

        if (LengthFlag && (LengthCounter == 0)) {
            return false;
        }

        if (!ForceVolume && !Volume) {
            return false;
        }

        if (!PlayBack) {
            return false;
        }

        return true;
    }

    i16 GetSample() override {
        u32 TrueVolume = ForceVolume ? 12 : Volume;

        u32 i = PositionCounter >> 1;

        u8 Sample = WaveRAM[i];

        if (!(PositionCounter & 1)) {
            // first the MSBs, then the LSBs
            Sample >>= 4;
        }
        else {
            Sample &= 0xf;
        }

        return -0x7fff + ((0xffff * TrueVolume * Sample) >> 8);  // normalized: /16 for Volume, /16 for Sample
    }

    void OnTick() override {
        PositionCounter = (PositionCounter + 1) & 0x1f;  // mod 32
        if (DoubleBanked && !PositionCounter) {
            SwitchBanks(BankNumber ^ 1);
        }
    }
};