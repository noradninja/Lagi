#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "audio/soundDriver.h"

// Vita host adapter for Azel's sound-driver API.
//
// The upstream soundDriver.cpp currently embeds desktop libao/SoLoud and an
// SCSP emulation backend. Lagi's platform layer owns audio output instead.
// With LAGI_NULL_AUDIO enabled we preserve Azel's call surface and timing
// expectations without introducing desktop audio dependencies.
//
// This file is a platform adapter only; game code continues to call the
// upstream soundDriver API unchanged.

void initSoundDriver()
{
    lagi::platform::audio::init();
}

void updateSoundInterrupt()
{
}

void updateSound()
{
    lagi::platform::audio::update();
}

void loadSoundBanks(s8, s8)
{
}

void playPCM(p_workArea, u32)
{
}

void enqueuePlaySoundEffect(s32, s32, s32, s32)
{
}

s32 playBattleSoundEffect(s32)
{
    return 0;
}

s32 fadeOutAllSequences()
{
    return 0;
}

s32 findSound(s32)
{
    return -1;
}

bool isSoundLoadingFinished()
{
    return true;
}

void popSoundSequence(s32)
{
}

s32 computePositionalSoundVolume(sVec3_FP*)
{
    return 0x7f;
}

void startPositionalSound(s32, sVec3_FP*)
{
}

void updatePositionalSound(s32, sVec3_FP*)
{
}

void setSoundDistanceParams(s32, s16)
{
}

void battleLoading_InitSub0()
{
}
