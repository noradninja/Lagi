#include "lagi/platform.h"
#include "../../integration/lagi_dsp_platform.h"

#include <cstdio>

extern "C" void lagi_audio_stop_worker_for_shutdown();

namespace lagi::platform {

static bool g_running = false;

bool init()
{
    if (!filesystem::init())
        return false;

    // Logging remains non-fatal so a filesystem problem cannot prevent boot.
    const bool logReady = logging::init();

    if (!input::init() || !renderer::init() || !audio::init())
        return false;

    if (logReady) {
        renderer::status("[PASS] LOG ux0:data/lagi/lagi.log", 0xFF30E030u);
    } else {
        char line[78];
        std::snprintf(
            line, sizeof(line),
            "[FAIL] LOG OPEN 0X%08X",
            static_cast<unsigned int>(logging::last_error()));
        renderer::failure(line);
    }

    g_running = true;
    return true;
}

void shutdown()
{
    lagi_audio_stop_worker_for_shutdown();
    audio::shutdown();
    lagi_dsp_platform_shutdown();
    renderer::shutdown();
    logging::shutdown();
    g_running = false;
}

bool running()
{
    return g_running && !input::exit_requested();
}

void begin_frame()
{
    input::update();
    renderer::begin_frame();
}

void end_frame()
{
    audio::update();
    renderer::end_frame();
}

} // namespace lagi::platform
