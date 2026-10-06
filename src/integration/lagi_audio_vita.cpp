#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "lagi/disc_image.h"
#include "audio/soundDriver.h"
#include "audio/soundDataTable.h"
#include "common.h"
#include "commonOverlay.h"
#include "kernel/fileBundle.h"

#include <array>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <vector>

#include <psp2/kernel/cpu.h>
#include <psp2/kernel/threadmgr.h>

extern "C" {
#include "ao.h"
#include "eng_ssf/m68k.h"
#include "eng_ssf/scsp.h"
#include "eng_ssf/sat_hw.h"

extern volatile unsigned lagi_dsp_mix_serial;
extern volatile unsigned lagi_dsp_mix_steps;
extern volatile unsigned lagi_dsp_mix_iwt;
extern volatile unsigned lagi_dsp_mix_twt;
extern volatile unsigned lagi_dsp_mix_mrd;
extern volatile unsigned lagi_dsp_mix_mwt;
extern volatile unsigned lagi_dsp_mix_ewt;
extern volatile unsigned lagi_dsp_mix_adrl;
extern volatile unsigned lagi_dsp_mix_frcl;
extern volatile unsigned lagi_dsp_mix_yrl;
extern volatile unsigned lagi_dsp_mix_xinput;
extern volatile unsigned lagi_dsp_mix_yfrc;
extern volatile unsigned lagi_dsp_mix_ycoef;
extern volatile unsigned lagi_dsp_mix_yreg;
extern volatile unsigned lagi_dsp_mix_satshift;
extern volatile unsigned lagi_dsp_mix_wrapshift;
extern volatile unsigned lagi_dsp_mix_unpack;
extern volatile unsigned lagi_dsp_mix_pack;
extern volatile unsigned lagi_dsp_mix_noflr;
extern volatile unsigned lagi_dsp_mix_noflw;
}

namespace {

constexpr unsigned kScspRate = 44100;
constexpr unsigned kTargetQueuedFrames = 2048;
constexpr unsigned kRenderChunkFrames = 256;
constexpr int kM68kCyclesPerSample = (11300000 / 60) / 735;

struct ScspCommand {
    s8 bytes[16]{};
};

struct PendingSound {
    s32 soundIndex = -1;
    s32 bankIndex = 0;
    s32 volume = 0x7f;
    s32 arg = 0;
};

struct ActiveSound {
    s16 soundIndex = -1;
    s16 volume = 0x80;
};

enum class AudioEventType : std::uint8_t {
    LoadBanks,
    StopBanks,
    Sound,
    FadeAll
};

struct AudioEvent {
    AudioEventType type = AudioEventType::Sound;
    s32 a = 0;
    s32 b = 0;
    s32 c = 0;
    s32 d = 0;
};

constexpr unsigned kAudioEventCapacity = 256;
constexpr unsigned kAudioEventMask = kAudioEventCapacity - 1;

std::atomic<unsigned long long> g_audioTraceSerial{0};
bool g_scspInitialized = false;
const sSequenceConfig* g_sequence = nullptr;
s32 g_sequenceNumber = -1;
s32 g_loadingState = -1;
s32 g_maxDistance = 0x200000;
s16 g_baseVolume = 100;
std::array<ActiveSound, 8> g_activeSounds{};
std::vector<ScspCommand> g_commands;
std::vector<PendingSound> g_pendingSounds;
std::array<std::int16_t, kRenderChunkFrames * 2> g_mixBuffer{};
std::array<AudioEvent, kAudioEventCapacity> g_audioEvents{};
std::atomic<unsigned> g_audioEventWrite{0};
std::atomic<unsigned> g_audioEventRead{0};
std::atomic<bool> g_audioWorkerStop{false};
std::atomic<bool> g_audioWorkerRunning{false};
std::atomic<bool> g_loadingFinished{true};
std::array<std::atomic<s32>, 8> g_activeSoundIds{};
SceUID g_audioWorkerThread = -1;
bool g_gameplayRenderEnabled = false;
std::atomic<unsigned> g_updateSoundCalls{0};
unsigned g_driverServiceCalls = 0;
unsigned g_audioDiagBudget = 24;

// Profiling only: these counters/timers never participate in emulation,
// scheduling, queue sizing, or command delivery.
unsigned g_audioPerfBudget = 16;
unsigned long long g_audioRenderChunks = 0;
unsigned long long g_audioQueueEmptyChunks = 0;
unsigned long long g_audioShortWrites = 0;
unsigned g_lastDspMixSerial = 0;

unsigned long long next_audio_trace()
{
    return g_audioTraceSerial.fetch_add(1, std::memory_order_relaxed) + 1;
}

void log_dsp_program_profile_if_changed()
{
    const unsigned serial = static_cast<unsigned>(lagi_dsp_mix_serial);
    if (serial == 0 || serial == g_lastDspMixSerial)
        return;

    g_lastDspMixSerial = serial;
    lagi::platform::logging::writef(
        "[AzelAudioDSP] serial=%u seq=%d steps=%u "
        "IWT=%u TWT=%u MRD=%u MWT=%u EWT=%u "
        "ADRL=%u FRCL=%u YRL=%u XINPUT=%u "
        "YFRC=%u YCOEF=%u YREG=%u "
        "shiftSat=%u shiftWrap=%u "
        "unpack=%u pack=%u noflR=%u noflW=%u\n",
        serial,
        static_cast<int>(g_sequenceNumber),
        static_cast<unsigned>(lagi_dsp_mix_steps),
        static_cast<unsigned>(lagi_dsp_mix_iwt),
        static_cast<unsigned>(lagi_dsp_mix_twt),
        static_cast<unsigned>(lagi_dsp_mix_mrd),
        static_cast<unsigned>(lagi_dsp_mix_mwt),
        static_cast<unsigned>(lagi_dsp_mix_ewt),
        static_cast<unsigned>(lagi_dsp_mix_adrl),
        static_cast<unsigned>(lagi_dsp_mix_frcl),
        static_cast<unsigned>(lagi_dsp_mix_yrl),
        static_cast<unsigned>(lagi_dsp_mix_xinput),
        static_cast<unsigned>(lagi_dsp_mix_yfrc),
        static_cast<unsigned>(lagi_dsp_mix_ycoef),
        static_cast<unsigned>(lagi_dsp_mix_yreg),
        static_cast<unsigned>(lagi_dsp_mix_satshift),
        static_cast<unsigned>(lagi_dsp_mix_wrapshift),
        static_cast<unsigned>(lagi_dsp_mix_unpack),
        static_cast<unsigned>(lagi_dsp_mix_pack),
        static_cast<unsigned>(lagi_dsp_mix_noflr),
        static_cast<unsigned>(lagi_dsp_mix_noflw));
}

bool push_audio_event(const AudioEvent& event)
{
    const unsigned write = g_audioEventWrite.load(std::memory_order_relaxed);
    const unsigned read = g_audioEventRead.load(std::memory_order_acquire);
    if (write - read >= kAudioEventCapacity) {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] audio event queue overflow type=%u\n",
            next_audio_trace(),
            static_cast<unsigned>(event.type));
        return false;
    }

    g_audioEvents[write & kAudioEventMask] = event;
    g_audioEventWrite.store(write + 1, std::memory_order_release);
    return true;
}

bool pop_audio_event(AudioEvent& event)
{
    const unsigned read = g_audioEventRead.load(std::memory_order_relaxed);
    const unsigned write = g_audioEventWrite.load(std::memory_order_acquire);
    if (read == write)
        return false;

    event = g_audioEvents[read & kAudioEventMask];
    g_audioEventRead.store(read + 1, std::memory_order_release);
    return true;
}

void reset_active_sounds()
{
    for (auto& sound : g_activeSounds) {
        sound.soundIndex = -1;
        sound.volume = 0x80;
    }
    for (auto& id : g_activeSoundIds)
        id.store(-1, std::memory_order_release);
}

void ensure_output_stream()
{
    if (!lagi::platform::audio::pcm_stream_active()) {
        if (lagi::platform::audio::start_pcm_stream(kScspRate, 2)) {
            lagi::platform::logging::writef(
                "[AzelAudio:%llu] SCSP output opened rate=%u stereo\n",
                next_audio_trace(), kScspRate);
        } else {
            lagi::platform::logging::writef(
                "[AzelAudio:%llu] SCSP output open FAILED\n",
                next_audio_trace());
        }
    }
}

void queue_command(
    u8 type,
    u8 p1 = 0,
    u8 p2 = 0,
    u8 p3 = 0,
    u8 p4 = 0,
    u8 p5 = 0)
{
    ScspCommand command{};
    command.bytes[0] = static_cast<s8>(type);
    command.bytes[2] = static_cast<s8>(p1);
    command.bytes[3] = static_cast<s8>(p2);
    command.bytes[4] = static_cast<s8>(p3);
    command.bytes[5] = static_cast<s8>(p4);
    command.bytes[6] = static_cast<s8>(p5);
    g_commands.push_back(command);
}

void write_driver_command(const ScspCommand& command, unsigned slot)
{
    const unsigned outputOffset = 0x700u + 0x10u * slot;
    for (unsigned i = 0; i < 16; ++i) {
        m68k_write_memory_8(
            outputOffset + i,
            static_cast<unsigned char>(command.bytes[i]));
    }
}

sSaturnPtr player_sound_bank_config(const sSequenceConfig* config)
{
    switch (config ? config->mD_playerSoundTypes : 0) {
    case 0:
        if (mainGameState.gameStats.m1_dragonLevel > 8)
            return readSaturnEA(gCommonFile->getSaturnPtr(0x213D90));
        return readSaturnEA(
            gCommonFile->getSaturnPtr(0x213D90) +
            mainGameState.gameStats.m1_dragonLevel * 4);
    case 1:
        return gCommonFile->getSaturnPtr(0x213DC0);
    case 2:
        return gCommonFile->getSaturnPtr(0x213DE4);
    default:
        return sSaturnPtr();
    }
}

bool load_sound_file_list(sSaturnPtr config)
{
    if (config.isNull())
        return false;

    while (readSaturnU32(config)) {
        const std::string filename =
            readSaturnString(readSaturnEA(config + 0));
        const u32 destination = readSaturnU32(config + 4);
        if (destination < 0x25A00000u ||
            destination - 0x25A00000u >= sizeof(sat_ram)) {
            lagi::platform::logging::writef(
                "[AzelAudio:%llu] SCSP invalid load destination file=%s dest=%08X\n",
                next_audio_trace(), filename.c_str(),
                static_cast<unsigned>(destination));
            return false;
        }

        const u32 offset = destination - 0x25A00000u;
        std::vector<std::uint8_t> data;
        if (!lagi::disc::read_file(filename.c_str(), data) ||
            data.empty() ||
            static_cast<std::size_t>(offset) + data.size() > sizeof(sat_ram)) {
            lagi::platform::logging::writef(
                "[AzelAudio:%llu] SCSP missing/oversize bank file=%s dest=%05X size=%u\n",
                next_audio_trace(), filename.c_str(),
                static_cast<unsigned>(offset),
                static_cast<unsigned>(data.size()));
            return false;
        }

        std::memcpy(sat_ram + offset, data.data(), data.size());
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP loaded %s -> %05X bytes=%u\n",
            next_audio_trace(), filename.c_str(),
            static_cast<unsigned>(offset),
            static_cast<unsigned>(data.size()));
        config += 0xC;
    }
    return true;
}

void load_current_banks()
{
    if (!g_sequence)
        return;

    const bool sequenceOk = load_sound_file_list(g_sequence->m0);
    const bool playerOk =
        load_sound_file_list(player_sound_bank_config(g_sequence));

    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SCSP banks sequence=%d sequenceOk=%d playerOk=%d\n",
        next_audio_trace(), static_cast<int>(g_sequenceNumber),
        sequenceOk ? 1 : 0, playerOk ? 1 : 0);
}

bool advance_loading_state(unsigned& commandSlot)
{
    if (!g_sequence || g_loadingState < 0)
        return false;

    switch (g_loadingState) {
    case 0: {
        // Match Azel updateSoundLoadingState(): loader commands are written
        // directly into the current 8-entry mailbox before queued SFX/fades.
        ScspCommand command{};
        command.bytes[0] = 0x10;
        command.bytes[2] = 1; // stop sequences
        command.bytes[3] = 1; // stop PCM
        command.bytes[4] = 1; // stop CD-DA
        command.bytes[5] = 1; // init DSP
        command.bytes[6] = 1; // init mixer
        write_driver_command(command, commandSlot++);
        g_loadingState = 1;
        return true;
    }
    case 1: {
        ScspCommand command{};
        command.bytes[0] = 0x08; // area-map change
        command.bytes[2] =
            static_cast<s8>(g_sequence->m8_areaMapIndex);
        write_driver_command(command, commandSlot++);
        g_loadingState = 2;
        return true;
    }
    case 2:
        // Upstream checkDataAndStopAllSequences() is currently reconstructed
        // as a no-op.
        g_loadingState = 3;
        return false;
    case 3:
        // Upstream waitForStopSoundCompletion() currently returns true.
        g_loadingState = 4;
        return false;
    case 4:
        // Upstream performs the disc-to-sound-RAM transfer from updateSound()
        // while the interrupt side waits in state 4.
        load_current_banks();
        g_loadingState = 5;
        return false;
    case 5: {
        ScspCommand command{};
        command.bytes[0] = static_cast<s8>(0x83); // effect change
        command.bytes[2] = 0;
        write_driver_command(command, commandSlot++);
        g_loadingState = 6;
        return true;
    }
    case 6: {
        ScspCommand command{};
        command.bytes[0] = static_cast<s8>(0x87); // mixer change
        command.bytes[2] = 0;
        write_driver_command(command, commandSlot++);
        g_loadingState = 7;
        return true;
    }
    case 7:
        for (unsigned i = 0; i < g_sequence->mC_numMapEntries; ++i)
            m68k_write_memory_8(0x504u + 8u * i, 0x80);
        g_loadingState = 8;
        return false;
    case 8:
        g_loadingState = -1;
        g_loadingFinished.store(true, std::memory_order_release);
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP sequence=%d ready\n",
            next_audio_trace(), static_cast<int>(g_sequenceNumber));
        return false;
    default:
        g_loadingState = -1;
        g_loadingFinished.store(true, std::memory_order_release);
        return false;
    }
}

void service_driver_commands()
{
    if (!g_scspInitialized)
        return;

    ++g_driverServiceCalls;

    const u8 timing = static_cast<u8>(m68k_read_memory_8(0x4E0));
    if (g_audioDiagBudget != 0 &&
        (g_driverServiceCalls <= 12 || g_loadingState >= 0)) {
        --g_audioDiagBudget;
        lagi::platform::logging::writef(
            "[AzelAudioDiag] service=%u update=%u load=%d pc=%06X "
            "flag4E0=%02X flag4E1=%02X cmds=%u pending=%u queued=%u\n",
            g_driverServiceCalls,
            g_updateSoundCalls.load(std::memory_order_relaxed),
            static_cast<int>(g_loadingState),
            static_cast<unsigned>(m68k_get_reg(nullptr, M68K_REG_PC)),
            static_cast<unsigned>(timing),
            static_cast<unsigned>(m68k_read_memory_8(0x4E1)),
            static_cast<unsigned>(g_commands.size()),
            static_cast<unsigned>(g_pendingSounds.size()),
            static_cast<unsigned>(
                lagi::platform::audio::queued_pcm_frames()));
    }

    // Upstream Azel only advances the loading state or writes a new mailbox
    // after the 68K clears bit 7 of the timing flag.
    if ((timing & 0x80u) != 0u)
        return;

    unsigned commandSlot = 0;
    const bool loaderWroteCommand =
        advance_loading_state(commandSlot);

    // Exactly like updateSoundInterrupt(), loader traffic gets first use of
    // the mailbox and normal queued commands fill the remaining slots.
    const unsigned available = 8u - commandSlot;
    const unsigned count = static_cast<unsigned>(
        std::min<std::size_t>(available, g_commands.size()));
    for (unsigned i = 0; i < count; ++i)
        write_driver_command(g_commands[i], commandSlot + i);

    if (count != 0u)
        g_commands.erase(g_commands.begin(), g_commands.begin() + count);

    if (loaderWroteCommand || count != 0u)
        m68k_write_memory_8(0x4E0, timing | 0x80u);
}

sSaturnPtr sound_config_for(s16 soundIndex)
{
    if (!g_sequence || !gCommonFile)
        return sSaturnPtr();

    if (soundIndex > 99) {
        return g_sequence->m4_soundConfigs +
            static_cast<u32>(soundIndex - 100) * 4u;
    }

    return gCommonFile->getSaturnPtr(0x02131C4) +
        static_cast<u32>(soundIndex) * 4u;
}

s32 find_active_sound(s32 soundIndex)
{
    for (unsigned i = 0; i < g_activeSounds.size(); ++i) {
        if (g_activeSounds[i].soundIndex == soundIndex)
            return static_cast<s32>(i);
    }
    return -1;
}

void submit_sound_request(const PendingSound& request)
{
    if (!g_sequence || request.soundIndex < 0)
        return;

    if (request.bankIndex == 1) {
        const sSaturnPtr config =
            sound_config_for(static_cast<s16>(request.soundIndex));
        if (config.isNull())
            return;

        const s8 sequenceDataBank = readSaturnS8(config + 0);
        const s8 soundNumber = readSaturnS8(config + 1);
        const s8 soundControlId = readSaturnS8(config + 3);
        if (sequenceDataBank < 0 || sequenceDataBank >= 8)
            return;

        ActiveSound& active =
            g_activeSounds[static_cast<unsigned>(sequenceDataBank)];
        if (request.volume != active.volume) {
            queue_command(
                0x05,
                static_cast<u8>(soundControlId),
                static_cast<u8>(request.volume),
                0);
            active.volume = static_cast<s16>(request.volume);
        }

        queue_command(
            0x01,
            static_cast<u8>(soundControlId),
            static_cast<u8>(sequenceDataBank),
            static_cast<u8>(soundNumber));
        active.soundIndex = static_cast<s16>(request.soundIndex);
        g_activeSoundIds[static_cast<unsigned>(sequenceDataBank)].store(
            request.soundIndex, std::memory_order_release);

        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP start sound=%d control=%d seqBank=%d seqSound=%d vol=%d\n",
            next_audio_trace(),
            static_cast<int>(request.soundIndex),
            static_cast<int>(soundControlId),
            static_cast<int>(sequenceDataBank),
            static_cast<int>(soundNumber),
            static_cast<int>(request.volume));
        return;
    }

    const s32 found = find_active_sound(request.soundIndex);
    if (found < 0)
        return;

    switch (request.bankIndex) {
    case 2:
    case 3:
    case 4:
        queue_command(
            static_cast<u8>(request.bankIndex),
            static_cast<u8>(found));
        break;
    case 5:
        queue_command(
            0x05,
            static_cast<u8>(found),
            static_cast<u8>(request.volume),
            static_cast<u8>(request.arg));
        g_activeSounds[static_cast<unsigned>(found)].volume =
            static_cast<s16>(request.volume);
        break;
    default:
        break;
    }
}

void service_pending_sounds()
{
    if (g_loadingState >= 0 || !g_sequence)
        return;

    for (const PendingSound& request : g_pendingSounds)
        submit_sound_request(request);
    g_pendingSounds.clear();
}

void render_scsp_audio()
{
    if (!g_scspInitialized || !g_sequence)
        return;

    ensure_output_stream();
    if (!lagi::platform::audio::pcm_stream_active())
        return;

    /*
     * Render at most one 256-frame quantum per worker iteration.
     *
     * When SCSP synthesis is slower than real time, an unbounded "fill to
     * target" loop can monopolize CPU2 forever: queued audio drains faster
     * than we can replenish it, so control never returns to
     * process_audio_events(). That makes newly enqueued SFX wait seconds.
     *
     * The outer worker loop immediately comes back here while the queue is
     * below target, so this does not add a sleep or alter emulation timing; it
     * only guarantees event/command service between render quanta.
     */
    if (lagi::platform::audio::queued_pcm_frames() <
        kTargetQueuedFrames) {
        ++g_audioRenderChunks;

        // Profile only a small, bounded set of chunks after each bank load,
        // plus one sparse steady-state sample. Sampling 1/8 of frames keeps
        // timer overhead low while preserving the exact emulation order.
        const bool profileChunk =
            g_audioPerfBudget != 0 ||
            ((g_audioRenderChunks & 0xFFu) == 0u);
        std::uint64_t renderStartUs = 0;
        std::uint64_t sampledM68kUs = 0;
        std::uint64_t sampledScspUs = 0;
        unsigned sampledFrames = 0;
        if (profileChunk)
            renderStartUs = sceKernelGetSystemTimeWide();

        const unsigned queuedBefore =
            static_cast<unsigned>(
                lagi::platform::audio::queued_pcm_frames());
        if (queuedBefore == 0)
            ++g_audioQueueEmptyChunks;

        int peak = 0;
        for (unsigned i = 0; i < kRenderChunkFrames; ++i) {
            const bool profileSample =
                profileChunk && ((i & 7u) == 0u);
            std::uint64_t t = 0;
            if (profileSample)
                t = sceKernelGetSystemTimeWide();

            m68k_execute(kM68kCyclesPerSample);

            if (profileSample) {
                sampledM68kUs += sceKernelGetSystemTimeWide() - t;
                t = sceKernelGetSystemTimeWide();
            }

            stereo_sample_t sample{};
            SCSP_Update(nullptr, nullptr, &sample);
            log_dsp_program_profile_if_changed();

            if (profileSample) {
                sampledScspUs += sceKernelGetSystemTimeWide() - t;
                ++sampledFrames;
            }

            g_mixBuffer[i * 2 + 0] = sample.l;
            g_mixBuffer[i * 2 + 1] = sample.r;
            const int al = sample.l < 0 ? -static_cast<int>(sample.l) : static_cast<int>(sample.l);
            const int ar = sample.r < 0 ? -static_cast<int>(sample.r) : static_cast<int>(sample.r);
            peak = std::max(peak, std::max(al, ar));
        }

        if (profileChunk) {
            const std::uint64_t renderUs =
                sceKernelGetSystemTimeWide() - renderStartUs;
            const std::uint64_t scale =
                sampledFrames != 0 ? (kRenderChunkFrames / sampledFrames) : 0;
            const std::uint64_t estimatedM68kUs =
                sampledM68kUs * scale;
            const std::uint64_t estimatedScspUs =
                sampledScspUs * scale;

            if (g_audioPerfBudget != 0)
                --g_audioPerfBudget;

            lagi::platform::logging::writef(
                "[AzelAudioPerf] seq=%d chunk=%llu frames=%u "
                "queued=%u peak=%d total=%lluus budget=%lluus "
                "m68k=%lluus scsp=%lluus dspSteps=%d samples=%u "
                "emptyChunks=%llu shortWrites=%llu\n",
                static_cast<int>(g_sequenceNumber),
                g_audioRenderChunks,
                kRenderChunkFrames,
                queuedBefore,
                peak,
                static_cast<unsigned long long>(renderUs),
                static_cast<unsigned long long>(
                    (1000000ull * kRenderChunkFrames) / kScspRate),
                static_cast<unsigned long long>(estimatedM68kUs),
                static_cast<unsigned long long>(estimatedScspUs),
                static_cast<int>(SCSP.DSP.LastStep),
                sampledFrames,
                g_audioQueueEmptyChunks,
                g_audioShortWrites);
        }

        if (g_audioDiagBudget != 0) {
            --g_audioDiagBudget;
            lagi::platform::logging::writef(
                "[AzelAudioDiag] render pc=%06X flag4E0=%02X peak=%d queuedBefore=%u load=%d\n",
                static_cast<unsigned>(m68k_get_reg(nullptr, M68K_REG_PC)),
                static_cast<unsigned>(m68k_read_memory_8(0x4E0)),
                peak,
                queuedBefore,
                static_cast<int>(g_loadingState));
        }

        const std::size_t written =
            lagi::platform::audio::write_pcm_frames(
                g_mixBuffer.data(), kRenderChunkFrames);
        if (written != kRenderChunkFrames) {
            ++g_audioShortWrites;
            lagi::platform::logging::writef(
                "[AzelAudioPerf] PCM short write requested=%u written=%u "
                "queued=%u shortWrites=%llu\n",
                kRenderChunkFrames,
                static_cast<unsigned>(written),
                static_cast<unsigned>(
                    lagi::platform::audio::queued_pcm_frames()),
                g_audioShortWrites);
            break;
        }

        // The 68K may have acknowledged a command while this block rendered.
        service_driver_commands();
    }
}

void clock_scsp_discard()
{
    // Movies own SceAudioOut, but the Saturn sound CPU must continue running.
    // Advance one small audio-time slice and discard the generated PCM so the
    // 68K can acknowledge mailbox commands and complete fades/stops.
    constexpr unsigned kDiscardFrames = 256;
    for (unsigned i = 0; i < kDiscardFrames; ++i) {
        m68k_execute(kM68kCyclesPerSample);
        stereo_sample_t sample{};
        SCSP_Update(nullptr, nullptr, &sample);
        log_dsp_program_profile_if_changed();
    }

    service_driver_commands();

    // 256 frames at 44.1 kHz is ~5.8 ms. Account approximately for the work
    // above while avoiding a busy-spin on CPU2 during FMV playback.
    sceKernelDelayThread(5000);
}

void worker_load_banks(s32 musicNumber, s32 mode)
{
    if (musicNumber < 0) {
        g_sequence = nullptr;
        g_sequenceNumber = -1;
        g_loadingState = -1;
        g_loadingFinished.store(true, std::memory_order_release);
        g_pendingSounds.clear();
        reset_active_sounds();
        g_gameplayRenderEnabled = false;
        for (u8 i = 0; i < 8; ++i)
            queue_command(0x02, i);
        return;
    }

    const std::size_t index = static_cast<std::size_t>(musicNumber);
    if (index >= SoundDataTable.size())
        return;

    g_sequence = &SoundDataTable[index];
    g_sequenceNumber = musicNumber;
    g_loadingState = 0;
    g_audioDiagBudget = 24;
    g_audioPerfBudget = 16;
    g_loadingFinished.store(false, std::memory_order_release);
    g_pendingSounds.clear();
    reset_active_sounds();
    g_gameplayRenderEnabled = true;
    ensure_output_stream();

    lagi::platform::logging::writef(
        "[AzelAudio:%llu] worker accepted sequence=%d mode=%d cpu=%d staleCmds=%u\n",
        next_audio_trace(),
        static_cast<int>(musicNumber),
        static_cast<int>(mode),
        sceKernelGetCpuId(),
        static_cast<unsigned>(g_commands.size()));
}

void worker_fade_all()
{
    for (u8 i = 0; i < 8; ++i) {
        queue_command(0x02, i);
        queue_command(0x05, i, 0x7f, 0);
    }

    // Module/movie transitions call this immediately before movie ownership.
    // Stop synthesizing into the shared PCM service until the next bank load.
    g_gameplayRenderEnabled = false;
}

void process_audio_events()
{
    AudioEvent event{};
    while (pop_audio_event(event)) {
        switch (event.type) {
        case AudioEventType::LoadBanks:
            worker_load_banks(event.a, event.b);
            break;
        case AudioEventType::StopBanks:
            worker_load_banks(-1, event.b);
            break;
        case AudioEventType::Sound:
            if (g_pendingSounds.size() >= 32)
                g_pendingSounds.erase(g_pendingSounds.begin());
            g_pendingSounds.push_back(
                PendingSound{event.a, event.b, event.c, event.d});
            break;
        case AudioEventType::FadeAll:
            worker_fade_all();
            break;
        }
    }
}

int audio_worker_thread(SceSize, void*)
{
    g_audioWorkerRunning.store(true, std::memory_order_release);
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SCSP worker started cpu=%d targetQueue=%u\n",
        next_audio_trace(),
        sceKernelGetCpuId(),
        kTargetQueuedFrames);

    while (!g_audioWorkerStop.load(std::memory_order_acquire)) {
        process_audio_events();

        service_driver_commands();
        service_pending_sounds();
        service_driver_commands();

        if (g_gameplayRenderEnabled && g_sequence) {
            render_scsp_audio();
            if (lagi::platform::audio::queued_pcm_frames() >=
                kTargetQueuedFrames) {
                sceKernelDelayThread(1000);
            }
        } else if (g_sequence) {
            clock_scsp_discard();
        } else {
            sceKernelDelayThread(1000);
        }
    }

    g_audioWorkerRunning.store(false, std::memory_order_release);
    return 0;
}

bool start_audio_worker()
{
    if (g_audioWorkerThread >= 0)
        return true;

    g_audioWorkerStop.store(false, std::memory_order_release);
    g_audioWorkerThread = sceKernelCreateThread(
        "LagiSCSPWorker",
        audio_worker_thread,
        0x10000100,
        128u * 1024u,
        0,
        SCE_KERNEL_CPU_MASK_USER_2,
        nullptr);
    if (g_audioWorkerThread < 0) {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP worker create FAILED rc=%d\n",
            next_audio_trace(),
            static_cast<int>(g_audioWorkerThread));
        return false;
    }

    const int startResult =
        sceKernelStartThread(g_audioWorkerThread, 0, nullptr);
    if (startResult < 0) {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP worker start FAILED rc=%d\n",
            next_audio_trace(),
            startResult);
        sceKernelDeleteThread(g_audioWorkerThread);
        g_audioWorkerThread = -1;
        return false;
    }

    return true;
}

void trace_sequence_config(s8 musicNumber, s8 mode)
{
    const int index = static_cast<int>(musicNumber);
    if (index >= 0 &&
        static_cast<std::size_t>(index) < SoundDataTable.size()) {
        const sSequenceConfig& config =
            SoundDataTable[static_cast<std::size_t>(index)];
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] BGM loadSoundBanks music=%d mode=%d "
            "areaMap=%d aux=%d mapEntries=%u playerSoundTypes=%u delta=%u flags=%u\n",
            next_audio_trace(), index, static_cast<int>(mode),
            static_cast<int>(config.m8_areaMapIndex),
            static_cast<int>(config.mA),
            static_cast<unsigned>(config.mC_numMapEntries),
            static_cast<unsigned>(config.mD_playerSoundTypes),
            static_cast<unsigned>(config.mE),
            static_cast<unsigned>(config.mF));
    } else {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] BGM loadSoundBanks music=%d mode=%d (stop/invalid sequence)\n",
            next_audio_trace(), index, static_cast<int>(mode));
    }
}

} // namespace

extern "C" int m68k_instructionCallback()
{
    return 0;
}

void initSoundDriver()
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] initSoundDriver -> native 68K/SCSP Vita service\n",
        next_audio_trace());

    lagi::platform::audio::init();
    std::memset(sat_ram, 0, sizeof(sat_ram));

    if (loadFile("SDDRVS.TSK", sat_ram, 0) < 0 ||
        loadFile("AREAMAP.SND", sat_ram + 0x0A000, 0) < 0) {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP bootstrap files missing\n",
            next_audio_trace());
        return;
    }

    sat_hw_init();
    m68k_write_memory_8(
        0x4E1, m68k_read_memory_8(0x4E1) | 0x80u);

    g_scspInitialized = true;
    reset_active_sounds();
    g_loadingFinished.store(true, std::memory_order_release);

    if (!start_audio_worker()) {
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP worker unavailable\n",
            next_audio_trace());
        return;
    }

    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SCSP core initialized cyclesPerSample=%d worker=CPU2\n",
        next_audio_trace(), kM68kCyclesPerSample);
    lagi::platform::logging::writef(
        "[AzelAudioPerf] profiler=outer-sampled sampleStride=8 "
        "steadyInterval=256 no-emulation-changes\n");
}

void updateSoundInterrupt()
{
    // 68K/SCSP ownership lives entirely on LagiSCSPWorker.
}

void updateSound()
{
    // Preserve Azel's frame-level call surface, but never synthesize audio on
    // the game thread. The worker continuously services queued commands and
    // keeps the native PCM ring filled.
    g_updateSoundCalls.fetch_add(1, std::memory_order_relaxed);
}

void loadSoundBanks(s8 musicNumber, s8 mode)
{
    trace_sequence_config(musicNumber, mode);

    AudioEvent event{};
    event.type = musicNumber < 0
        ? AudioEventType::StopBanks
        : AudioEventType::LoadBanks;
    event.a = static_cast<s32>(musicNumber);
    event.b = static_cast<s32>(mode);
    g_loadingFinished.store(
        musicNumber < 0, std::memory_order_release);
    push_audio_event(event);
}

void playPCM(p_workArea, u32 id)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX playPCM task id=%u\n",
        next_audio_trace(), static_cast<unsigned>(id));
    enqueuePlaySoundEffect(static_cast<s32>(id), 1, 0x7f, 0);
}

void lagiAzelFieldPlayPCM(const char* filename)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] PCM field file=%s (stream path pending)\n",
        next_audio_trace(), filename ? filename : "<null>");
}

void enqueuePlaySoundEffect(
    s32 soundIndex,
    s32 bankIndex,
    s32 volume,
    s32 arg)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SFX enqueue sound=%d bank=%d volume=%d arg=%d\n",
        next_audio_trace(), static_cast<int>(soundIndex),
        static_cast<int>(bankIndex), static_cast<int>(volume),
        static_cast<int>(arg));

    if (soundIndex < 0)
        return;

    AudioEvent event{};
    event.type = AudioEventType::Sound;
    event.a = soundIndex;
    event.b = bankIndex;
    event.c = volume;
    event.d = arg;
    push_audio_event(event);
}

s32 playBattleSoundEffect(s32 effectIndex)
{
    enqueuePlaySoundEffect(effectIndex, 2, 0, 0);
    return 0;
}

s32 fadeOutAllSequences()
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] BGM fadeOutAllSequences\n",
        next_audio_trace());

    AudioEvent event{};
    event.type = AudioEventType::FadeAll;
    push_audio_event(event);
    return 0;
}

s32 findSound(s32 soundIndex)
{
    for (unsigned i = 0; i < g_activeSoundIds.size(); ++i) {
        if (g_activeSoundIds[i].load(std::memory_order_acquire) == soundIndex)
            return static_cast<s32>(i);
    }
    return -1;
}

bool isSoundLoadingFinished()
{
    return g_loadingFinished.load(std::memory_order_acquire);
}

void popSoundSequence(s32 param)
{
    lagi::platform::logging::writef(
        "[AzelAudio:%llu] BGM popSoundSequence param=%d\n",
        next_audio_trace(), static_cast<int>(param));
}

s32 computePositionalSoundVolume(sVec3_FP* position)
{
    if (!position || g_maxDistance <= 0)
        return 0x7f;

    fixedPoint distSq = MTH_Product3d_FP(*position, *position);
    const s32 distance = static_cast<s32>(sqrt_F(distSq));
    s32 result =
        (static_cast<s32>(g_baseVolume) - 0x7f) *
        distance / g_maxDistance + 0x7f;
    result = std::max<s32>(0, std::min<s32>(0x7f, result));
    return result;
}

void startPositionalSound(s32 soundIndex, sVec3_FP* position)
{
    enqueuePlaySoundEffect(
        soundIndex, 1,
        computePositionalSoundVolume(position), 0);
}

void updatePositionalSound(s32 soundIndex, sVec3_FP* position)
{
    enqueuePlaySoundEffect(
        soundIndex, 5,
        computePositionalSoundVolume(position), 0);
}

void setSoundDistanceParams(s32 maxDistance, s16 baseVolume)
{
    g_maxDistance = maxDistance;
    g_baseVolume = baseVolume;
}

void battleLoading_InitSub0()
{
}
