#include "APU.h"

#include "log.h"

#include <cmath>

GBAAPU::GBAAPU(s_scheduler* scheduler, u8* wave_ram_ptr, const std::function<void(u32)>& fifo_callback) :
    sq{Square(scheduler), Square(scheduler)},
    ns(scheduler),
    wav(scheduler, wave_ram_ptr),
    fifo{FIFOChannel(fifo_callback, 0x0400'00a0), FIFOChannel(fifo_callback, 0x0400'00a4)}
{

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
            AUDIO_F32SYS, 2, 44100
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
    APU->FrameSequencer++;
    switch (APU->FrameSequencer & 7) {
        case 2:
        case 6:
            APU->sq[0].DoSweep();
        case 0:
        case 4:
            APU->sq[0].TickLengthCounter();
            APU->sq[1].TickLengthCounter();
            APU->ns.TickLengthCounter();
            APU->wav.TickLengthCounter();
            break;
        case 7:
            APU->sq[0].DoEnvelope();
            APU->sq[1].DoEnvelope();
            APU->ns.DoEnvelope();
            break;
        default:
            break;
    }

    event->time += FrameSequencerPeriod;
    scheduler->AddEvent(event);
}

static float tester = 0;
void GBAAPU::DoProvideSample() {
    if (!Stream) {
        return;
    }

    if(SDL_AudioStreamAvailable(Stream) > 0.5 * SampleFrequency * sizeof(float)) {
        // more than half a second of audio available, stop providing samples
        return;
    }

    i32 SampleLeft = 0;
    i32 SampleRight = 0;

    SampleLeft += sq[0].CurrentSample;
    SampleLeft += sq[1].CurrentSample;
    SampleLeft += ns.CurrentSample;
    SampleLeft += wav.CurrentSample;
    SampleLeft += fifo[0].CurrentSample * 8;
    SampleLeft += fifo[1].CurrentSample * 8;

    SampleRight += sq[0].CurrentSample;
    SampleRight += sq[1].CurrentSample;
    SampleRight += ns.CurrentSample;
    SampleRight += wav.CurrentSample;
    SampleRight += fifo[0].CurrentSample * 8;
    SampleRight += fifo[1].CurrentSample * 8;

    i16 samples[2] = {
            (i16)((i32)(SampleLeft  * ExternalVolume) >> 5),  // left
            (i16)((i32)(SampleRight * ExternalVolume) >> 5),  // right
    };

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