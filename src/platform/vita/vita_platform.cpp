#include "lagi/platform.h"

namespace lagi::platform {

static bool g_running = false;

bool init()
{
    if (!filesystem::init())
        return false;

    // Logging is intentionally non-fatal: the runtime can still boot if the
    // log file cannot be created, while stdout remains available.
    logging::init();

    if (!input::init() || !renderer::init() || !audio::init())
        return false;

    g_running = true;
    return true;
}

void shutdown()
{
    audio::shutdown();
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
