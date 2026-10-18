#include "lagi/lagi_input_bridge.h"

#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/platform.h"
#include "lagi/battle_stick_input.h"
#if LAGI_BATTLE_BUILD_EXECUTE_A3
#include "battle/battleManager.h"
#endif

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

    bool battleRunning = false;
#if LAGI_BATTLE_BUILD_EXECUTE_A3
    battleRunning = gBattleManager && gBattleManager->m0_status == 3;
#endif
    const auto directions = battle_stick_directions(
        platform::input::analog_x(), platform::input::analog_y(), battleRunning);
    const auto previous = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m6_buttonDown;
    pending.m6_buttonDown |= directions;
    // Compare the combined controls, so swapping stick/D-pad while held does
    // not introduce a second press. Azel owns repeat timing and battle actions.
    pending.m8_newButtonDown = pending.m6_buttonDown & ~previous;
    pending.mC_newButtonDown2 = pending.m8_newButtonDown;

    const float analogX = -platform::input::analog_x();
    const float analogY = -platform::input::analog_y();
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
