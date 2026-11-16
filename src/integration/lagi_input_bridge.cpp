#include "lagi/lagi_input_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "lagi/battle_stick_input.h"

#include "VDP1.h"

#if LAGI_RECORD_AZEL_INPUT
#include "kernel/moduleManager.h"
#include "lagi/disc_image.h"
#include <psp2/io/stat.h>

#include <cstdint>
#include <cstdio>
#include <psp2/io/fcntl.h>

namespace {

struct InputSample
{
    std::uint32_t frame;
    std::uint16_t held;
    std::uint16_t pressed;
    std::int8_t x;
    std::int8_t y;
    std::uint16_t status;
    std::uint8_t inputType;
    std::uint8_t rider2;
    std::uint8_t disc;
    std::uint8_t reserved;
};
static_assert(sizeof(InputSample) == 16);

bool writeTrace(SceUID file, const void* data, unsigned size)
{
    auto* bytes = static_cast<const char*>(data);
    while (size != 0)
    {
        const int written = sceIoWrite(file, bytes, size);
        if (written <= 0)
            return false;
        bytes += written;
        size -= static_cast<unsigned>(written);
    }
    return true;
}

SceUID openTrace()
{
    constexpr char header[] = "LAGIINP1";
    for (unsigned slot = 1; slot <= 999; ++slot)
    {
        char path[64];
        std::snprintf(path, sizeof(path), "ux0:data/lagi/input-%03u.bin", slot);
        SceIoStat existing{};
        if (sceIoGetstat(path, &existing) >= 0)
            continue;
        const SceUID file = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_EXCL, 0666);
        if (file >= 0)
        {
            if (writeTrace(file, header, sizeof(header) - 1))
            {
                lagi::platform::logging::writef("[InputTrace] recording %s\n", path);
                return file;
            }
            sceIoClose(file);
        }
        break;
    }
    lagi::platform::logging::writef("[InputTrace] could not open trace\n");
    return -1;
}

void recordInput()
{
    static SceUID file = openTrace();
    if (file < 0)
        return;

    static InputSample samples[60];
    static unsigned count = 0;
    static std::uint32_t frame = 0;
    const auto& pad = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
    samples[count++] = {
        frame++, pad.m6_buttonDown, pad.m8_newButtonDown,
        pad.m2_analogX, pad.m3_analogY, static_cast<std::uint16_t>(gGameStatus.m4_gameStatus),
        static_cast<std::uint8_t>(pad.m0_inputType), mainGameState.gameStats.m3_rider2,
        static_cast<std::uint8_t>(lagi::disc::selected_disc()), 0
    };
    if (count == sizeof(samples) / sizeof(samples[0]))
    {
        if (!writeTrace(file, samples, sizeof(samples)))
        {
            lagi::platform::logging::writef("[InputTrace] write failed at frame %u\n", frame);
            sceIoClose(file);
            file = -1;
        }
        count = 0;
    }
}

} // namespace
#endif

void updateInputs();

namespace lagi::input_bridge {

void sync_to_azel()
{
    auto& pending =
        graphicEngineStatus.m4514.m0_inputDevices[0].m16_pending;
    pending.m0_inputType = 2;
    pending.m6_buttonDown = platform::input::saturn_buttons_down();
    pending.m8_newButtonDown = platform::input::saturn_buttons_pressed();
    pending.mC_newButtonDown2 = platform::input::saturn_buttons_pressed();

    const auto directions = stick_directions(
        platform::input::analog_x(), platform::input::analog_y());
    const auto previous = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m6_buttonDown;
    pending.m6_buttonDown |= directions;
    // Compare the combined controls, so swapping stick/D-pad while held does
    // not introduce a second press. Azel owns repeat timing and battle actions.
    pending.m8_newButtonDown = pending.m6_buttonDown & ~previous;
    pending.mC_newButtonDown2 = pending.m8_newButtonDown;

    // Conversely, expose the D-pad through Azel's analog fields. Prefer the
    // physical stick on an axis while it is outside the dead zone.
    const float stickX = platform::input::analog_x();
    const float stickY = platform::input::analog_y();
    const float sourceX = stickX != 0.0f
        ? stickX : static_cast<float>(platform::input::digital_x());
    const float sourceY = stickY != 0.0f
        ? stickY : -static_cast<float>(platform::input::digital_y());
    const float analogX = -sourceX;
    const float analogY = -sourceY;
    pending.m2_analogX = static_cast<s8>(
        analogX <= -1.0f ? -127 :
        analogX >= 1.0f ? 127 :
        analogX * 127.0f);
    pending.m3_analogY = static_cast<s8>(
        analogY <= -1.0f ? -127 :
        analogY >= 1.0f ? 127 :
        analogY * 127.0f);

    updateInputs();
#if LAGI_RECORD_AZEL_INPUT
    recordInput();
#endif
}

} // namespace lagi::input_bridge
