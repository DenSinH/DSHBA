#include "APU.h"

#include "log.h"

#include <cmath>

GBAAPU::GBAAPU(s_scheduler* scheduler) {
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
    // log_debug("Requesting %d samples", length / sizeof(float));

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
            *out++ = sample; // (float)std::sin(8 * sizeof(float) * (double)i * 3.141592 / ((double)length)) / 20.0;
        }
    }
}

SCHEDULER_EVENT(GBAAPU::TickFrameSequencerEvent) {
    auto APU = (GBAAPU*)caller;

    event->time += FrameSequencerPeriod;
    scheduler->AddEvent(event);
}

SCHEDULER_EVENT(GBAAPU::ProvideSampleEvent) {
    auto APU = (GBAAPU*)caller;

    if (APU->Stream) {
        i16 samples[2] = {
                0,  // left
                0,  // right
        };

        SDL_AudioStreamPut(APU->Stream, samples, 2 * sizeof(i16));
    }

    event->time += SamplePeriod;
    scheduler->AddEvent(event);
}

void GBAAPU::AudioDestroy() {

}