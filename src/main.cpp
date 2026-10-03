#include <psp2/kernel/processmgr.h>
#include <cstdio>
#include "lagi/platform.h"
#include "lagi/lagi_runtime.h"

int main()
{
    std::printf("Lagi - native PDS runtime for PlayStation Vita\n");

    if (!lagi::platform::init()) {
        std::printf("Platform initialization failed.\n");
        sceKernelExitProcess(1);
        return 1;
    }

    lagi::platform::logging::writef(
        "Lagi - native PDS runtime for PlayStation Vita\n");
    lagi::platform::logging::writef(
        "Platform initialized.\nData root: %s\nPDS data: %s\n",
        lagi::platform::filesystem::data_root(),
        lagi::platform::filesystem::game_data_present()
            ? "present" : "not detected");
    lagi::platform::logging::writef(
        "Persistent log: %s\n", lagi::platform::logging::path());

    lagi::platform::renderer::status(
        "[PASS] VITA FRAMEBUFFER / PLATFORM", 0xFF30E030u);

    if (!lagi::azel::runtime_smoke_init()) {
        lagi::platform::logging::writef(
            "Azel runtime smoke init failed.\n");
        lagi::platform::shutdown();
        sceKernelExitProcess(2);
        return 2;
    }

    lagi::platform::logging::writef(
        "Azel runtime: native boot task graph active\n");

    while (lagi::platform::running()) {
        lagi::platform::begin_frame();
        lagi::azel::runtime_smoke_frame();
        lagi::platform::end_frame();
    }

    lagi::platform::shutdown();
    sceKernelExitProcess(0);
    return 0;
}
