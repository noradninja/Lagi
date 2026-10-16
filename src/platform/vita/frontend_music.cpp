#include "lagi/platform.h"
#include <psp2/kernel/threadmgr.h>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cstdint>
#define MINIMP3_IMPLEMENTATION
#define MINIMP3_ONLY_MP3
#include "third_party/minimp3.h"

namespace lagi::platform::frontend_music {
namespace {
std::atomic<bool> stopRequested{false};
SceUID worker = -1;
constexpr const char* kPath = "ux0:data/lagi/frontend/bgm.mp3";
constexpr int kBufferCapacity = 16384;

int run(SceSize, void*)
{
    FILE* file = std::fopen(kPath, "rb");
    if (!file) return 0;
    mp3dec_t decoder{};
    mp3dec_init(&decoder);
    unsigned char encoded[kBufferCapacity]{};
    mp3d_sample_t decoded[MINIMP3_MAX_SAMPLES_PER_FRAME]{};
    int buffered = 0;
    bool audioReady = false;
    unsigned sampleRate = 0, channels = 0;
    logging::writef("[FrontendMusic] opened %s\n", kPath);
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
        unsigned sent = 0;
        while (sent < static_cast<unsigned>(samples) &&
               !stopRequested.load(std::memory_order_acquire)) {
            sent += static_cast<unsigned>(audio::write_pcm_frames(
                decoded + sent * channels, static_cast<unsigned>(samples) - sent));
            if (sent < static_cast<unsigned>(samples))
                sceKernelDelayThread(2000);
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
    FILE* test = std::fopen(kPath, "rb");
    if (!test) {
        logging::writef("[FrontendMusic] optional file absent: %s\n", kPath);
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
    logging::writef("[FrontendMusic] output released before Azel\n");
}
} // namespace lagi::platform::frontend_music
