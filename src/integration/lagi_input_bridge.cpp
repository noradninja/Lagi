#include "lagi/lagi_input_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "lagi/battle_stick_input.h"

#include "VDP1.h"

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
}

} // namespace lagi::input_bridge
