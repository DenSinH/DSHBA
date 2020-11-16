#include "APU.h"

#include "log.h"

#include <cmath>

GBAAPU::GBAAPU(s_scheduler* scheduler) : sq{Square(scheduler), Square(scheduler)} {

    this->Scheduler = scheduler;

    TickFrameSequencer = (s_event) {
        .callback = TickFrameSequencerEvent,
        .caller = this,
        .time = 0,
    };
    scheduler->AddEvent(&TickFrameSequencer);

    ProvideSample = (s_event) {
        .callback = ProvideSampleEvent,
        .caller = this,
        .time = SamplePeriod
    };

    scheduler->AddEvent(&ProvideSample);
}

void GBAAPU::AudioInit() {
    Device = SDL_OpenAudioDevice(nullptr, 0, &GBASpec, &ActualSpec, 0);

    if (!Device) {
        log_warn("Failed to open audio device: %s", SDL_GetError());
    }

    Stream = SDL_NewAudioStream(
            AUDIO_S16SYS, 2, SampleFrequency,
            AUDIO_F32SYS, 2, 48000
    );  // thank you Dillon
    SDL_PauseAudioDevice(Device, false);
}

void GBAAPU::AudioCallback(void *apu, u8 *stream, int length) {
    auto APU = (GBAAPU*)apu;

    int gotten = 0;
    if (SDL_AudioStreamAvailable(APU->Stream)) {
        // buffer samples we provided
        gotten = SDL_AudioStreamGet(APU->Stream, stream, length);
    }

    if (gotten < length) {
        int gotten_samples = gotten / sizeof(float);
        float* out = ((float*)stream) + gotten_samples;

        for (int i = gotten_samples; i < length / sizeof(float); i++) {
            float sample = 0;  // todo: last sample the APU generated
            *out++ = sample;
        }
    }
}

SCHEDULER_EVENT(GBAAPU::TickFrameSequencerEvent) {
    auto APU = (GBAAPU*)caller;

    event->time += FrameSequencerPeriod;
    scheduler->AddEvent(event);
}

static float tester = 0;
void GBAAPU::DoProvideSample() {
    if (!Stream) {
        return;
    }

    float SampleLeft = 0;
    float SampleRight = 0;

    SampleLeft += (float)sq[0].CurrentSample;
    SampleLeft += (float)sq[1].CurrentSample;

    SampleRight += (float)sq[0].CurrentSample;
    SampleRight += (float)sq[1].CurrentSample;

    i16 samples[2] = {
            (i16)(SampleLeft  * ExternalVolume / 8),  // left
            (i16)(SampleRight * ExternalVolume / 8),  // right
    };

    // log_debug("Providing %d", samples[0]);

    SDL_AudioStreamPut(Stream, samples, 2 * sizeof(i16));
}

SCHEDULER_EVENT(GBAAPU::ProvideSampleEvent) {
    auto APU = (GBAAPU*)caller;

    APU->DoProvideSample();

    event->time += SamplePeriod;
    scheduler->AddEvent(event);
}

void GBAAPU::AudioDestroy() {

}