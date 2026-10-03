#include "lagi/platform.h"
#include <psp2/ctrl.h>

namespace lagi::platform::input {

static bool g_exit = false;
static unsigned int g_previousButtons = 0;
static unsigned short g_saturnButtonsDown = 0;
static unsigned short g_saturnButtonsPressed = 0;
static float g_analogX = 0.0f;
static float g_analogY = 0.0f;
static float g_analogZoom = 0.0f;
static float g_analogCameraX = 0.0f;
static float g_analogCameraY = 0.0f;
static int g_digitalX = 0;
static int g_digitalY = 0;
static bool g_runHeld = false;
static bool g_cameraHeld = false;
static bool g_resetScene = false;
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
    g_analogCameraX = axis(pad.rx);
    g_analogCameraY = axis(pad.ry);
    g_digitalX =
        (buttons & SCE_CTRL_RIGHT ? 1 : 0) -
        (buttons & SCE_CTRL_LEFT ? 1 : 0);
    g_digitalY =
        (buttons & SCE_CTRL_UP ? 1 : 0) -
        (buttons & SCE_CTRL_DOWN ? 1 : 0);
    // Vita -> Saturn physical pad mapping. Azel's own buttonConfig tables
    // remain responsible for translating these physical bits into walking,
    // dragon and battle actions.
    unsigned short saturn = 0;
    if (buttons & SCE_CTRL_SQUARE)    saturn |= 0x0001; // Saturn A
    if (buttons & SCE_CTRL_CROSS)     saturn |= 0x0002; // Saturn B
    if (buttons & SCE_CTRL_CIRCLE)    saturn |= 0x0004; // Saturn C
    if (buttons & SCE_CTRL_START)     saturn |= 0x0008; // Start
    if (buttons & SCE_CTRL_UP)        saturn |= 0x0010;
    if (buttons & SCE_CTRL_DOWN)      saturn |= 0x0020;
    if (buttons & SCE_CTRL_RTRIGGER)  saturn |= 0x1000; // Saturn R
    if (buttons & SCE_CTRL_TRIANGLE)  saturn |= 0x4000; // Saturn Y
    if (buttons & SCE_CTRL_LTRIGGER)  saturn |= 0x8000; // Saturn L

    const bool debugComboHeld =
        (buttons & SCE_CTRL_START) && (buttons & SCE_CTRL_SELECT);
    const bool debugComboPressed =
        debugComboHeld &&
        (pressed & (SCE_CTRL_START | SCE_CTRL_SELECT));

    // Start+Select is a Lagi-only debug chord. Do not leak Start into Azel on
    // the chord frame, otherwise gameplay may also react to the same press.
    if (debugComboHeld)
        saturn &= static_cast<unsigned short>(~0x0008u);

    g_saturnButtonsPressed =
        static_cast<unsigned short>(saturn & ~g_saturnButtonsDown);
    g_saturnButtonsDown = saturn;

    // Legacy diagnostic controls are retired now that the authentic Azel
    // runtime owns gameplay input.
    g_runHeld = false;
    g_cameraHeld = false;
    g_resetScene = false;
    g_prevMode = (pressed & SCE_CTRL_LEFT) != 0;
    g_nextMode = (pressed & SCE_CTRL_RIGHT) != 0;

    if (debugComboPressed) {
        renderer::toggle_full_debug_screen();
    } else if (pressed & SCE_CTRL_SELECT) {
        renderer::toggle_debug_console();
    }

    g_previousButtons = buttons;
}

bool exit_requested() { return g_exit; }
unsigned short saturn_buttons_down() { return g_saturnButtonsDown; }
unsigned short saturn_buttons_pressed() { return g_saturnButtonsPressed; }
float analog_x() { return g_analogX; }
float analog_y() { return g_analogY; }
float analog_zoom() { return g_analogZoom; }
float analog_camera_x() { return g_analogCameraX; }
float analog_camera_y() { return g_analogCameraY; }
int digital_x() { return g_digitalX; }
int digital_y() { return g_digitalY; }
bool run_held() { return g_runHeld; }
bool camera_held() { return g_cameraHeld; }
bool reset_scene_pressed() { return g_resetScene; }
bool prev_mode_pressed() { return g_prevMode; }
bool next_mode_pressed() { return g_nextMode; }

} // namespace lagi::platform::input
