#include "lagi/platform.h"
#include <psp2/ctrl.h>

namespace lagi::platform::input {

static bool g_exit = false;
static unsigned int g_previousButtons = 0;
static float g_analogX = 0.0f;
static float g_analogY = 0.0f;
static float g_analogZoom = 0.0f;
static int g_digitalX = 0;
static int g_digitalY = 0;
static bool g_runHeld = false;
static bool g_resetView = false;
static bool g_prevMode = false;
static bool g_nextMode = false;

static float axis(unsigned char v)
{
    const int centered = static_cast<int>(v) - 128;
    const int dead = 18;
    if (centered > -dead && centered < dead)
        return 0.0f;
    const float f = static_cast<float>(centered) / 127.0f;
    return f < -1.0f ? -1.0f : (f > 1.0f ? 1.0f : f);
}

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

    g_analogX = axis(pad.lx);
    g_analogY = axis(pad.ly);
    g_analogZoom = axis(pad.ry);
    g_digitalX =
        (buttons & SCE_CTRL_RIGHT ? 1 : 0) -
        (buttons & SCE_CTRL_LEFT ? 1 : 0);
    g_digitalY =
        (buttons & SCE_CTRL_UP ? 1 : 0) -
        (buttons & SCE_CTRL_DOWN ? 1 : 0);
    g_runHeld = (buttons & SCE_CTRL_CROSS) != 0;
    g_resetView = (pressed & SCE_CTRL_TRIANGLE) != 0;
    g_prevMode = (pressed & SCE_CTRL_LTRIGGER) != 0;
    g_nextMode = (pressed & SCE_CTRL_RTRIGGER) != 0;

    if ((buttons & SCE_CTRL_START) && (buttons & SCE_CTRL_SELECT)) {
        g_exit = true;
    } else if (pressed & SCE_CTRL_SELECT) {
        renderer::toggle_debug_console();
    }

    g_previousButtons = buttons;
}

bool exit_requested() { return g_exit; }
float analog_x() { return g_analogX; }
float analog_y() { return g_analogY; }
float analog_zoom() { return g_analogZoom; }
int digital_x() { return g_digitalX; }
int digital_y() { return g_digitalY; }
bool run_held() { return g_runHeld; }
bool reset_view_pressed() { return g_resetView; }
bool prev_mode_pressed() { return g_prevMode; }
bool next_mode_pressed() { return g_nextMode; }

} // namespace lagi::platform::input
