#pragma once

#include "Channels/Square.h"

#include "../Scheduler/scheduler.h"

#include "default.h"
#include "const.h"

#include <SDL.h>

#define AUDIO_BUFFER_SIZE 1024

class GBAAPU {
public:

    explicit GBAAPU(s_scheduler* scheduler);

    void AudioInit();
    void AudioDestroy();

    float ExternalVolume = 0.05;

private:
    friend class Initializer;

    static const int SampleFrequency = 32768;

    /* Internal function */

    s_scheduler* Scheduler;
    u32 FrameSequencer;
    static const u32 FrameSequencerPeriod = 0x8000;  // CPU cycles
    static const u32 SamplePeriod = CLOCK_FREQUENCY / SampleFrequency;

    static SCHEDULER_EVENT(TickFrameSequencerEvent);
    s_event TickFrameSequencer;
    static SCHEDULER_EVENT(ProvideSampleEvent);
    void DoProvideSample();
    s_event ProvideSample;

    Square sq[2];

    /* External function */
    static void AudioCallback(void* apu, u8* stream, int length);

    const SDL_AudioSpec GBASpec = {
        .freq     = SampleFrequency,
        .format   = AUDIO_F32SYS,
        .channels = 2,
        .samples  = AUDIO_BUFFER_SIZE,
        .callback = AudioCallback,
        .userdata = this,
    };

    SDL_AudioSpec ActualSpec;
    SDL_AudioDeviceID Device;
    SDL_AudioStream* Stream = nullptr;
};