#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "audio/soundDriver.h"
#include "audio/soundDataTable.h"

// Vita host adapter for Azel's sound-driver API.
//
// The upstream soundDriver.cpp currently embeds desktop libao/SoLoud and an
// SCSP emulation backend. Lagi's platform layer owns audio output instead.
// With LAGI_NULL_AUDIO enabled we preserve Azel's call surface and timing
// expectations without introducing desktop audio dependencies.
//
// This file is a platform adapter only; game code continues to call the
// upstream soundDriver API unchanged.

namespace {

unsigned long long g_audioTraceSerial = 0;

unsigned long long next_audio_trace()
{
    return ++g_audioTraceSerial;
}

void trace_sequence_config(s8 musicNumber, s8 unk1)
{
    const unsigned long long serial = next_audio_trace();
    const int index = static_cast<int>(musicNumber);
    if (index >= 0 && static_cast<std::size_t>(index) < SoundDataTable.size()) {
        const sSequenceConfig& config = SoundDataTable[static_cast<std::size_t>(index)];
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] BGM loadSoundBanks music=%d mode=%d "
            "areaMap=%d aux=%d mapEntries=%u playerSoundTypes=%u delta=%u flags=%u\n",
            serial,
            index,
            static_cast<int>(unk1),
            static_cast<int>(config.m8_areaMapIndex),
            static_cast<int>(config.mA),
            static_cast<unsigned int>(config.mC_numMapEntries),
            static_cast<unsigned int>(config.mD_playerSoundTypes),
            static_cast<unsigned int>(config.mE),
            static_cast<unsigned int>(config.mF));
    } else {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] BGM loadSoundBanks music=%d mode=%d (stop/invalid sequence)\n",
            serial,
            index,
            static_cast<int>(unk1));
    }
}

} // namespace

void initSoundDriver()
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] initSoundDriver -> Vita audio service\n",
        next_audio_trace());
    lagi::platform::audio::init();
}

void updateSoundInterrupt()
{
}

void updateSound()
{
    lagi::platform::audio::update();
}

void loadSoundBanks(s8 musicNumber, s8 unk1)
{
    trace_sequence_config(musicNumber, unk1);
}

void playPCM(p_workArea, u32 id)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX playPCM task id=%u\n",
        next_audio_trace(),
        static_cast<unsigned int>(id));
}

void lagiAzelFieldPlayPCM(const char* filename)
{
    // Field-script PCM is a platform audio service. Keep Azel's script
    // sequencing intact and expose the exact request for native playback.
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] PCM field file=%s\n",
        next_audio_trace(),
        filename ? filename : "<null>");
}

void enqueuePlaySoundEffect(s32 soundIndex, s32 bankIndex, s32 volume, s32 unk)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX enqueue sound=%d bank=%d volume=%d arg=%d\n",
        next_audio_trace(),
        static_cast<int>(soundIndex),
        static_cast<int>(bankIndex),
        static_cast<int>(volume),
        static_cast<int>(unk));
}

s32 playBattleSoundEffect(s32 effectIndex)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX battle sound=%d -> bank=2 volume=0\n",
        next_audio_trace(),
        static_cast<int>(effectIndex));
    return 0;
}

s32 fadeOutAllSequences()
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] BGM fadeOutAllSequences\n",
        next_audio_trace());
    return 0;
}

s32 findSound(s32 soundIndex)
{
    // This can be queried repeatedly by gameplay. Leave it quiet for the
    // playthrough trace; the actual playback requests carry the useful event.
    (void)soundIndex;
    return -1;
}

bool isSoundLoadingFinished()
{
    return true;
}

void popSoundSequence(s32 param)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] BGM popSoundSequence param=%d\n",
        next_audio_trace(),
        static_cast<int>(param));
}

s32 computePositionalSoundVolume(sVec3_FP*)
{
    return 0x7f;
}

void startPositionalSound(s32 soundIndex, sVec3_FP*)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX positional-start sound=%d\n",
        next_audio_trace(),
        static_cast<int>(soundIndex));
}

void updatePositionalSound(s32, sVec3_FP*)
{
    // Deliberately not traced: this may execute every frame for active sounds.
}

void setSoundDistanceParams(s32 maxDistance, s16 baseVolume)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX distance max=%d baseVolume=%d\n",
        next_audio_trace(),
        static_cast<int>(maxDistance),
        static_cast<int>(baseVolume));
}

void battleLoading_InitSub0()
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] battleLoading_InitSub0\n",
        next_audio_trace());
}
