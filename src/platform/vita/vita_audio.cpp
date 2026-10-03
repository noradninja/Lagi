#include "lagi/platform.h"

#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>

namespace lagi::platform::audio {
namespace {

constexpr std::size_t kRingSamples = 1u << 17;
constexpr std::size_t kOutputFrames = 1024;
constexpr std::size_t kRingMask = kRingSamples - 1u;

std::array<std::int16_t, kRingSamples> g_ring{};
std::array<std::int16_t, kOutputFrames * 2u> g_output{};
std::atomic<std::uint64_t> g_writePosition{0};
std::atomic<std::uint64_t> g_readPosition{0};
std::atomic<std::uint64_t> g_playedFrames{0};
std::atomic<std::uint64_t> g_submittedFrames{0};
std::atomic<bool> g_stopRequested{false};
std::atomic<bool> g_inputFinished{false};
std::atomic<bool> g_threadRunning{false};
SceUID g_thread = -1;
int g_port = -1;
unsigned int g_channels = 0;

std::size_t queued_samples()
{
    const std::uint64_t write = g_writePosition.load(std::memory_order_acquire);
    const std::uint64_t read = g_readPosition.load(std::memory_order_acquire);
    return static_cast<std::size_t>(write - read);
}

int audio_thread(SceSize, void*)
{
    g_threadRunning.store(true, std::memory_order_release);
    const std::size_t blockSamples = kOutputFrames * g_channels;
    bool outputStarted = false;

    while (!g_stopRequested.load(std::memory_order_acquire)) {
        std::size_t available = queued_samples();
        const bool finished = g_inputFinished.load(std::memory_order_acquire);

        // Do not start the hardware clock until one complete block is ready.
        if (!outputStarted && available < blockSamples && !finished) {
            sceKernelDelayThread(1000);
            continue;
        }
        if (finished && available == 0)
            break;

        outputStarted = true;
        const std::size_t consume = std::min(available, blockSamples);
        std::uint64_t read = g_readPosition.load(std::memory_order_relaxed);
        for (std::size_t i = 0; i < consume; ++i)
            g_output[i] = g_ring[static_cast<std::size_t>(read + i) & kRingMask];
        std::fill(g_output.begin() + consume,
                  g_output.begin() + blockSamples,
                  static_cast<std::int16_t>(0));
        g_readPosition.store(read + consume, std::memory_order_release);

        if (sceAudioOutOutput(g_port, g_output.data()) < 0)
            break;
        const std::uint64_t submitted =
            g_submittedFrames.fetch_add(
                kOutputFrames, std::memory_order_acq_rel) + kOutputFrames;
        const int remaining = sceAudioOutGetRestSample(g_port);
        if (remaining >= 0 && static_cast<std::uint64_t>(remaining) <= submitted)
            g_playedFrames.store(
                submitted - static_cast<std::uint64_t>(remaining),
                std::memory_order_release);
    }

    g_threadRunning.store(false, std::memory_order_release);
    return 0;
}

} // namespace

bool init()
{
    return true;
}

void shutdown()
{
    stop_pcm_stream();
}

void update()
{
}

bool start_pcm_stream(unsigned int sampleRate, unsigned int channels)
{
    stop_pcm_stream();
    if (!sampleRate || (channels != 1u && channels != 2u))
        return false;

    const SceAudioOutMode mode = channels == 1u
        ? SCE_AUDIO_OUT_MODE_MONO
        : SCE_AUDIO_OUT_MODE_STEREO;
    g_port = sceAudioOutOpenPort(
        SCE_AUDIO_OUT_PORT_TYPE_BGM,
        static_cast<int>(kOutputFrames),
        static_cast<int>(sampleRate),
        mode);
    if (g_port < 0)
        return false;

    g_channels = channels;
    g_writePosition.store(0, std::memory_order_relaxed);
    g_readPosition.store(0, std::memory_order_relaxed);
    g_playedFrames.store(0, std::memory_order_relaxed);
    g_submittedFrames.store(0, std::memory_order_relaxed);
    g_stopRequested.store(false, std::memory_order_relaxed);
    g_inputFinished.store(false, std::memory_order_relaxed);
    g_threadRunning.store(false, std::memory_order_relaxed);

    g_thread = sceKernelCreateThread(
        "LagiMovieAudio",
        audio_thread,
        0x10000100,
        64u * 1024u,
        0,
        0,
        nullptr);
    if (g_thread < 0) {
        sceAudioOutReleasePort(g_port);
        g_port = -1;
        g_channels = 0;
        return false;
    }

    if (sceKernelStartThread(g_thread, 0, nullptr) < 0) {
        sceKernelDeleteThread(g_thread);
        g_thread = -1;
        sceAudioOutReleasePort(g_port);
        g_port = -1;
        g_channels = 0;
        return false;
    }
    return true;
}

std::size_t write_pcm_frames(
    const std::int16_t* interleaved,
    std::size_t frames)
{
    if (g_port < 0 || !interleaved || !frames || !g_channels ||
        g_stopRequested.load(std::memory_order_acquire) ||
        g_inputFinished.load(std::memory_order_acquire))
        return 0;

    const std::uint64_t write = g_writePosition.load(std::memory_order_relaxed);
    const std::uint64_t read = g_readPosition.load(std::memory_order_acquire);
    const std::size_t used = static_cast<std::size_t>(write - read);
    const std::size_t freeSamples = kRingSamples - used;
    const std::size_t acceptedFrames = std::min(
        frames, freeSamples / g_channels);
    const std::size_t acceptedSamples = acceptedFrames * g_channels;

    for (std::size_t i = 0; i < acceptedSamples; ++i)
        g_ring[static_cast<std::size_t>(write + i) & kRingMask] = interleaved[i];
    g_writePosition.store(write + acceptedSamples, std::memory_order_release);
    return acceptedFrames;
}

void finish_pcm_stream()
{
    if (g_port >= 0)
        g_inputFinished.store(true, std::memory_order_release);
}

void stop_pcm_stream()
{
    if (g_thread >= 0) {
        g_stopRequested.store(true, std::memory_order_release);
        sceKernelWaitThreadEnd(g_thread, nullptr, nullptr);
        sceKernelDeleteThread(g_thread);
        g_thread = -1;
    }
    if (g_port >= 0) {
        sceAudioOutReleasePort(g_port);
        g_port = -1;
    }

    g_channels = 0;
    g_threadRunning.store(false, std::memory_order_relaxed);
    g_inputFinished.store(false, std::memory_order_relaxed);
    g_stopRequested.store(false, std::memory_order_relaxed);
    g_writePosition.store(0, std::memory_order_relaxed);
    g_readPosition.store(0, std::memory_order_relaxed);
    g_playedFrames.store(0, std::memory_order_relaxed);
    g_submittedFrames.store(0, std::memory_order_relaxed);
}

std::uint64_t played_pcm_frames()
{
    if (g_port >= 0) {
        const std::uint64_t submitted =
            g_submittedFrames.load(std::memory_order_acquire);
        const int remaining = sceAudioOutGetRestSample(g_port);
        if (remaining >= 0 && static_cast<std::uint64_t>(remaining) <= submitted) {
            const std::uint64_t played =
                submitted - static_cast<std::uint64_t>(remaining);
            g_playedFrames.store(played, std::memory_order_release);
            return played;
        }
    }
    return g_playedFrames.load(std::memory_order_acquire);
}

std::size_t queued_pcm_frames()
{
    return g_channels ? queued_samples() / g_channels : 0;
}

bool pcm_stream_drained()
{
    return g_port >= 0 &&
           g_inputFinished.load(std::memory_order_acquire) &&
           queued_samples() == 0 &&
           !g_threadRunning.load(std::memory_order_acquire);
}

bool pcm_stream_active()
{
    return g_port >= 0;
}

} // namespace lagi::platform::audio
