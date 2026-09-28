#include "lagi/platform.h"
#include <psp2/ctrl.h>

namespace lagi::platform::input {

static bool g_exit = false;
static unsigned int g_previousButtons = 0;

bool init()
{
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    g_previousButtons = 0;
    return true;
}

void update()
{
    SceCtrlData pad{};
    sceCtrlPeekBufferPositive(0, &pad, 1);

    const unsigned int buttons = pad.buttons;
    const unsigned int pressed = buttons & ~g_previousButtons;

    if ((buttons & SCE_CTRL_START) && (buttons & SCE_CTRL_SELECT)) {
        g_exit = true;
    } else if (pressed & SCE_CTRL_SELECT) {
        // Saturn has no SELECT button, so reserve Vita SELECT for Lagi debug UI.
        renderer::toggle_debug_console();
    }

    g_previousButtons = buttons;
}

bool exit_requested()
{
    return g_exit;
}

} // namespace lagi::platform::input
