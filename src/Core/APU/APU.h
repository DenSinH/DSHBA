#pragma once

#include "Channels/Square.h"
#include "Channels/Noise.h"
#include "Channels/Wave.h"
#include "Channels/FIFO.h"

#include "../Scheduler/scheduler.h"

#include "default.h"
#include "const.h"

#include <SDL.h>

#include <mutex>
#include <functional>

#define AUDIO_BUFFER_SIZE 1024

enum class SOUNDCNT_HFlags : u16 {
    PSGVolume       = 0x0003,
    DMAAVolume      = 0x0004,
    DMABVolume      = 0x0008,
    DMAAEnableRight = 0x0100,
    DMAAEnableLeft  = 0x0200,
    DMAATMSelect    = 0x0400,
    DMAAResetFIFO   = 0x0800,
    DMABEnableRight = 0x1000,
    DMABEnableLeft  = 0x2000,
    DMABTMSelect    = 0x4000,
    DMABResetFIFO   = 0x8000,
};

enum class SOUNDCNT_XFlags : u16 {
    PSG0Enabled  = 0x0001,
    PSG1Enabled  = 0x0002,
    PSG2Enabled  = 0x0004,
    PSG3Enabled  = 0x0008,
    MasterEnable = 0x0080,
};

class GBAAPU {
public:

    explicit GBAAPU(s_scheduler* scheduler, u8* wave_ram_ptr, const std::function<void(u32)>& fifo_callback);

    void AudioInit();
    void AudioDestroy();

    bool SyncToAudio = true;
    bool ExternalEnable = true;
    float ExternalVolume = 1.0;
    bool ExternalChannelEnable[4] = { true, true, true, true };
    bool ExternalFIFOEnable[2] = { true, true };
    float ExternalChannelVolume[4] = { 1.0, 1.0, 1.0, 1.0 };
    float ExternalFIFOVolume[2] = { 1.0, 1.0 };

private:
    friend class Initializer;
    friend class MMIO;  // MMIO needs full control over the APU

    static const int SampleFrequency = 32768;

    /* Internal function */

    s_scheduler* const Scheduler;
    u32 FrameSequencer = 0;
    static const u32 FrameSequencerPeriod = 0x8000;  // CPU cycles
    static const u32 SamplePeriod = CLOCK_FREQUENCY / SampleFrequency;

    static SCHEDULER_EVENT(TickFrameSequencerEvent);
    s_event* const TickFrameSequencer = Scheduler->MakeEvent(
            this, TickFrameSequencerEvent
    );
    static SCHEDULER_EVENT(ProvideSampleEvent);
    void WaitForAudioSync();
    void DoProvideSample();
    s_event* const ProvideSample = Scheduler->MakeEvent(
            this, ProvideSampleEvent
    );

    u8 MasterVolumeRight = 0;
    u8 MasterVolumeLeft = 0;
    u8 SoundEnableRight = 0;
    u8 SoundEnableLeft = 0;

    u16 SOUNDCNT_H;
    u16 SOUNDCNT_X;

    Square sq[2];
    Noise ns;
    Wave wav;
    FIFOChannel fifo[2];

    Channel* const channels[4] = {
            &sq[0], &sq[1], &ns, &wav
    };

    /* External function */
    static void AudioCallback(void* apu, u8* stream, int length);

    const SDL_AudioSpec GBASpec = {
        .freq     = 44100,
        .format   = AUDIO_F32SYS,
        .channels = 2,
        .samples  = AUDIO_BUFFER_SIZE,
        .callback = AudioCallback,
        .userdata = this,
    };

    std::mutex BufferMutex = std::mutex();
    SDL_AudioSpec ActualSpec;
    SDL_AudioDeviceID Device;
    SDL_AudioStream* Stream = nullptr;
};