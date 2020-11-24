#pragma once

#include "EnvelopeChannel.h"

class Square : public EnvelopeChannel {

public:
    explicit Square(s_scheduler* scheduler) : EnvelopeChannel(scheduler) {
        Period = 128 * 100;
        TruePeriod = 128 * 100;
    }

    void SetDuty(u8 index) {
        Duty = DutyCycles[index];
    }

    u32 SweepNumber = 0;
    bool SweepUp = false;
    u32 SweepPeriod = 0;
    i32 SweepTimer = 0;

    void SweepReload() {
        SweepTimer = SweepPeriod;
    }

    void DoSweep() {
        if (SweepPeriod == 0) {
            return;
        }

        if (--SweepTimer == 0) {
            i32 dPeriod = TruePeriod >> SweepNumber;
            if (!SweepUp) {
                dPeriod *= -1;
            }

            TruePeriod += dPeriod;
            // we dont want to underflow/overflow the period
            // setting a max on the period makes it so (in this case) for at most 1 second, the channel is frozen
            TruePeriod = std::clamp(TruePeriod, 1u, (u32)CLOCK_FREQUENCY);

            SetPeriod(TruePeriod);

            SweepTimer = SweepPeriod;

            UpdateEvent();
        }
    }

private:
    friend class Initializer;
    static constexpr const u8 DutyCycles[4]  = {
            0x80,  // 12.5%
            0xc0,  // 25%
            0xf0,  // 50%
            0xfc,  // 75%
    };

    u8 Index = 0;
    u8 Duty  = DutyCycles[0];

protected:

    i16 GetSample() override {
        if (Duty & (1 << Index)) {
            return (0x7fff * Volume) >> 4;
        }
        else {
            return (-0x7fff * Volume) >> 4;
        }
    }

    void OnTick() override {
        Index = (Index + 1) & 7;
    }
};