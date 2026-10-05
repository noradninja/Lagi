#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "audio/soundDriver.h"
#include "audio/soundDataTable.h"
#include "common.h"
#include "commonOverlay.h"
#include "kernel/fileBundle.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <vector>

extern "C" {
#include "ao.h"
#include "eng_ssf/m68k.h"
#include "eng_ssf/scsp.h"
#include "eng_ssf/sat_hw.h"
}

namespace {

constexpr unsigned kScspRate = 44100;
constexpr unsigned kTargetQueuedFrames = 4096;
constexpr unsigned kRenderChunkFrames = 1024;
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

unsigned long long g_audioTraceSerial = 0;
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

unsigned long long next_audio_trace()
{
    return ++g_audioTraceSerial;
}

void reset_active_sounds()
{
    for (auto& sound : g_activeSounds) {
        sound.soundIndex = -1;
        sound.volume = 0x80;
    }
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
    if (!config)
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
        if (loadFile(filename.c_str(), sat_ram + offset, 0) < 0) {
            lagi::platform::logging::writef(
                "[AzelAudio:%llu] SCSP missing bank file=%s dest=%05X\n",
                next_audio_trace(), filename.c_str(),
                static_cast<unsigned>(offset));
            return false;
        }

        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP loaded %s -> %05X\n",
            next_audio_trace(), filename.c_str(),
            static_cast<unsigned>(offset));
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

void advance_loading_state()
{
    if (!g_sequence || g_loadingState < 0)
        return;

    switch (g_loadingState) {
    case 0:
        // Saturn sound-init: stop sequences/PCM/CD-DA and reset DSP/mixer.
        queue_command(0x10, 1, 1, 1, 1, 1);
        g_loadingState = 1;
        break;
    case 1:
        // Area map change.
        queue_command(
            0x08,
            static_cast<u8>(g_sequence->m8_areaMapIndex));
        g_loadingState = 2;
        break;
    case 2:
        // Upstream's stop/wait helpers are currently reconstructed as no-ops.
        g_loadingState = 4;
        break;
    case 4:
        load_current_banks();
        g_loadingState = 5;
        break;
    case 5:
        queue_command(0x83, 0);
        g_loadingState = 6;
        break;
    case 6:
        queue_command(0x87, 0);
        g_loadingState = 7;
        break;
    case 7:
        for (unsigned i = 0; i < g_sequence->mC_numMapEntries; ++i)
            m68k_write_memory_8(0x504u + 8u * i, 0x80);
        g_loadingState = 8;
        break;
    case 8:
        g_loadingState = -1;
        lagi::platform::logging::writef(
            "[AzelAudio:%llu] SCSP sequence=%d ready\n",
            next_audio_trace(), static_cast<int>(g_sequenceNumber));
        break;
    default:
        g_loadingState = -1;
        break;
    }
}

void service_driver_commands()
{
    if (!g_scspInitialized)
        return;

    advance_loading_state();

    const u8 timing = static_cast<u8>(m68k_read_memory_8(0x4E0));
    if ((timing & 0x80u) != 0u || g_commands.empty())
        return;

    const unsigned count =
        static_cast<unsigned>(std::min<std::size_t>(8, g_commands.size()));
    for (unsigned i = 0; i < count; ++i)
        write_driver_command(g_commands[i], i);

    g_commands.erase(g_commands.begin(), g_commands.begin() + count);
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
        if (!config)
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

    while (lagi::platform::audio::queued_pcm_frames() <
           kTargetQueuedFrames) {
        for (unsigned i = 0; i < kRenderChunkFrames; ++i) {
            m68k_execute(kM68kCyclesPerSample);
            stereo_sample_t sample{};
            SCSP_Update(nullptr, nullptr, &sample);
            g_mixBuffer[i * 2 + 0] = sample.l;
            g_mixBuffer[i * 2 + 1] = sample.r;
        }

        const std::size_t written =
            lagi::platform::audio::write_pcm_frames(
                g_mixBuffer.data(), kRenderChunkFrames);
        if (written != kRenderChunkFrames)
            break;

        // The 68K may have acknowledged a command while this block rendered.
        service_driver_commands();
    }
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
    ensure_output_stream();

    lagi::platform::logging::writef(
        "[AzelAudio:%llu] SCSP core initialized cyclesPerSample=%d\n",
        next_audio_trace(), kM68kCyclesPerSample);
}

void updateSoundInterrupt()
{
    service_driver_commands();
}

void updateSound()
{
    service_driver_commands();
    service_pending_sounds();
    service_driver_commands();
    render_scsp_audio();
}

void loadSoundBanks(s8 musicNumber, s8 mode)
{
    trace_sequence_config(musicNumber, mode);

    if (musicNumber < 0) {
        g_sequence = nullptr;
        g_sequenceNumber = -1;
        g_loadingState = -1;
        g_pendingSounds.clear();
        reset_active_sounds();
        for (u8 i = 0; i < 8; ++i)
            queue_command(0x02, i);
        service_driver_commands();
        return;
    }

    const std::size_t index = static_cast<std::size_t>(musicNumber);
    if (index >= SoundDataTable.size())
        return;

    g_sequence = &SoundDataTable[index];
    g_sequenceNumber = musicNumber;
    g_loadingState = 0;
    g_pendingSounds.clear();
    reset_active_sounds();
    ensure_output_stream();
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

    if (g_pendingSounds.size() >= 32)
        g_pendingSounds.erase(g_pendingSounds.begin());

    g_pendingSounds.push_back(
        PendingSound{soundIndex, bankIndex, volume, arg});
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

    for (u8 i = 0; i < 8; ++i) {
        queue_command(0x02, i);
        queue_command(0x05, i, 0x7f, 0);
    }
    service_driver_commands();
    return 0;
}

s32 findSound(s32 soundIndex)
{
    return find_active_sound(soundIndex);
}

bool isSoundLoadingFinished()
{
    return g_loadingState < 0;
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
