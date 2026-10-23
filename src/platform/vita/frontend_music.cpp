#include "lagi/platform.h"
#include <psp2/kernel/threadmgr.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <vector>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "third_party/minimp3.h"

namespace lagi::platform::frontend_music {
namespace {
std::atomic<bool> stopRequested{false};
std::atomic<int> requestedCue{-1};
std::vector<std::int16_t> navigationPcm;
std::vector<std::int16_t> confirmPcm;
SceUID worker = -1;
constexpr const char* kPackaged = "app0:frontend/bgm.mp3";
constexpr const char* kLegacy = "ux0:data/lagi/frontend/bgm.mp3";
const char* g_musicPath = kPackaged;
FILE* open_music()
{
    FILE* f = std::fopen(kPackaged, "rb");
    g_musicPath = kPackaged;
    if (!f) { f = std::fopen(kLegacy, "rb"); g_musicPath = kLegacy; }
    return f;
}
constexpr int kBufferCapacity = 16384;
// Keep input-side music ahead of output by <100 ms so UI cues are responsive.
constexpr unsigned kFrontendQueueFrames = 4096;

std::uint32_t little32(const unsigned char* bytes)
{
    return static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8) |
        (static_cast<std::uint32_t>(bytes[2]) << 16) |
        (static_cast<std::uint32_t>(bytes[3]) << 24);
}

bool load_cue(const char* path, std::vector<std::int16_t>& pcm)
{
    FILE* file = std::fopen(path, "rb");
    if (!file) return false;
    unsigned char header[44]{};
    const bool valid = std::fread(header, 1, sizeof(header), file) == sizeof(header) &&
        std::memcmp(header, "RIFF", 4) == 0 &&
        std::memcmp(header + 8, "WAVEfmt ", 8) == 0 &&
        little32(header + 16) == 16 &&
        header[20] == 1 && header[21] == 0 &&
        header[22] == 2 && header[23] == 0 &&
        little32(header + 24) == 44100 &&
        header[34] == 16 && header[35] == 0 &&
        std::memcmp(header + 36, "data", 4) == 0;
    const std::uint32_t bytes = little32(header + 40);
    if (!valid || bytes == 0 || bytes > 44100u * 4u * 2u || bytes % 4u) {
        std::fclose(file);
        return false;
    }
    pcm.resize(bytes / sizeof(std::int16_t));
    const bool complete = std::fread(pcm.data(), 1, bytes, file) == bytes;
    std::fclose(file);
    if (!complete) pcm.clear();
    return complete;
}

std::int16_t clipped(int value)
{
    return static_cast<std::int16_t>(
        std::max(-32768, std::min(32767, value)));
}

int run(SceSize, void*)
{
    FILE* file = open_music();
    if (!file) return 0;
    mp3dec_t decoder{};
    mp3dec_init(&decoder);
    unsigned char encoded[kBufferCapacity]{};
    mp3d_sample_t decoded[MINIMP3_MAX_SAMPLES_PER_FRAME]{};
    int buffered = 0;
    bool audioReady = false;
    unsigned sampleRate = 0, channels = 0;
    const std::vector<std::int16_t>* activeCue = nullptr;
    std::size_t cueFrame = 0;
    logging::writef("[FrontendMusic] opened %s\n", g_musicPath);
    while (!stopRequested.load(std::memory_order_acquire)) {
        if (buffered < 4096) {
            const int amount = static_cast<int>(
                std::fread(encoded + buffered, 1, kBufferCapacity - buffered, file));
            buffered += amount;
            if (!amount && buffered < 4) {
                std::rewind(file);
                std::clearerr(file);
                mp3dec_init(&decoder);
                buffered = 0;
                continue;
            }
        }
        if (buffered < 4) break;
        mp3dec_frame_info_t info{};
        const int samples = mp3dec_decode_frame(
            &decoder, encoded, buffered, decoded, &info);
        if (info.frame_bytes <= 0) {
            if (std::feof(file)) {
                std::rewind(file);
                std::clearerr(file);
                mp3dec_init(&decoder);
                buffered = 0;
                continue;
            }
            sceKernelDelayThread(1000);
            continue;
        }
        buffered -= info.frame_bytes;
        std::memmove(encoded, encoded + info.frame_bytes, buffered);
        if (!samples) continue;
        if (!audioReady) {
            sampleRate = static_cast<unsigned>(info.hz);
            channels = static_cast<unsigned>(info.channels);
            if (!audio::start_pcm_stream(sampleRate, channels)) {
                logging::writef("[FrontendMusic] PCM output init failed\n");
                break;
            }
            logging::writef("[FrontendMusic] playing hz=%u channels=%u\n",
                            sampleRate, channels);
            audioReady = true;
        }
        if (static_cast<unsigned>(info.hz) != sampleRate ||
            static_cast<unsigned>(info.channels) != channels) break;
        const int cue = requestedCue.exchange(-1, std::memory_order_acq_rel);
        if (cue >= 0) {
            activeCue = cue == 0 ? &navigationPcm : &confirmPcm;
            cueFrame = 0;
        }
        if (activeCue && sampleRate == 44100) {
            const std::size_t cueFrames = activeCue->size() / 2;
            for (int i = 0; i < samples && cueFrame < cueFrames;
                 ++i, ++cueFrame) {
                const int left = (*activeCue)[cueFrame * 2];
                const int right = (*activeCue)[cueFrame * 2 + 1];
                decoded[i * channels] = clipped(decoded[i * channels] + left);
                if (channels == 2)
                    decoded[i * channels + 1] = clipped(
                        decoded[i * channels + 1] + right);
            }
            if (cueFrame >= cueFrames) activeCue = nullptr;
        }
        unsigned sent = 0;
        while (sent < static_cast<unsigned>(samples) &&
               !stopRequested.load(std::memory_order_acquire)) {
            const std::size_t queued = audio::queued_pcm_frames();
            if (queued >= kFrontendQueueFrames) {
                sceKernelDelayThread(2000);
                continue;
            }
            const unsigned remaining = static_cast<unsigned>(samples) - sent;
            const unsigned room = static_cast<unsigned>(
                kFrontendQueueFrames - queued);
            const unsigned accepted = static_cast<unsigned>(
                audio::write_pcm_frames(decoded + sent * channels,
                    std::min(remaining, room)));
            sent += accepted;
            if (!accepted) sceKernelDelayThread(2000);
        }
    }
    std::fclose(file);
    logging::writef("[FrontendMusic] worker stopped\n");
    return 0;
}
}
void start()
{
    if (worker >= 0) return;
    navigationPcm.clear();
    confirmPcm.clear();
    requestedCue.store(-1, std::memory_order_release);
    const bool navigationReady = load_cue(
        "app0:frontend/navigate.wav", navigationPcm);
    const bool confirmReady = load_cue(
        "app0:frontend/confirm.wav", confirmPcm);
    logging::writef("[FrontendMusic] original PCM cues navigation=%u confirm=%u\n",
                    navigationReady ? 1u : 0u, confirmReady ? 1u : 0u);
    FILE* test = open_music();
    if (!test) {
        logging::writef("[FrontendMusic] optional file absent: %s\n", g_musicPath);
        return;
    }
    std::fclose(test);
    stopRequested.store(false, std::memory_order_release);
    worker = sceKernelCreateThread("LagiFrontendMP3", run, 0x10000100,
                                   96 * 1024, 0, 0, nullptr);
    if (worker < 0) {
        logging::writef("[FrontendMusic] thread creation failed\n");
        return;
    }
    if (sceKernelStartThread(worker, 0, nullptr) < 0) {
        sceKernelDeleteThread(worker);
        worker = -1;
        logging::writef("[FrontendMusic] thread start failed\n");
    }
}
void stop()
{
    if (worker < 0) return;
    stopRequested.store(true, std::memory_order_release);
    sceKernelWaitThreadEnd(worker, nullptr, nullptr);
    sceKernelDeleteThread(worker);
    worker = -1;
    audio::stop_pcm_stream();
    navigationPcm.clear();
    confirmPcm.clear();
    requestedCue.store(-1, std::memory_order_release);
    logging::writef("[FrontendMusic] output released before Azel\n");
}
void play_navigation()
{
    if (!navigationPcm.empty())
        requestedCue.store(0, std::memory_order_release);
}
void play_confirm()
{
    if (!confirmPcm.empty())
        requestedCue.store(1, std::memory_order_release);
}
} // namespace lagi::platform::frontend_music
