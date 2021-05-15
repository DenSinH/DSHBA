#pragma once

#include "Channel.h"

#include <algorithm>

class EnvelopeChannel : public Channel {

    /*
     * PSG channel with volume envelope effect
     * */

public:
    explicit EnvelopeChannel(s_scheduler* scheduler) : Channel(scheduler) {}

    i32 EnvelopePeriod = 0;
    i32 EnvelopeTime = 0;
    bool EnvelopeUp = false;

    void DoEnvelope() {
        if (EnvelopePeriod) {
            EnvelopeTime--;
            Volume += EnvelopeUp ? 1 : -1;

            Volume = std::clamp((i32)Volume, 0, 16);
            if (!EnvelopeTime) {
                EnvelopeTime = EnvelopePeriod;
            }
        }
    }
};