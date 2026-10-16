#include <psp2/kernel/processmgr.h>
#include <cstdio>
#include <psp2/display.h>
#include "lagi/platform.h"
#include "lagi/lagi_runtime.h"
#include "lagi/disc_image.h"

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

    // The disc decision happens before Azel creates its native task graph.
    bool discAvailable[4]{};
    int selection = -1;
    for (int i = 0; i < 4; ++i) {
        discAvailable[i] = lagi::disc::disc_available(i + 1);
        if (selection < 0 && discAvailable[i])
            selection = i;
        lagi::platform::logging::writef(
            "[DiscSelector] Disc %d: %s\n",
            i + 1, discAvailable[i] ? "available" : "missing");
    }

    bool selectedDiscMounted = false;
    while (lagi::platform::running() && !selectedDiscMounted) {
        lagi::platform::input::update();
        const unsigned short pressed =
            lagi::platform::input::saturn_buttons_pressed();
        // The selector positions correspond to physical directions:
        // 0=top (Disc 1), 1=right (Disc 2), 2=bottom (Disc 3),
        // 3=left (Disc 4). Missing images cannot be selected.
        int direction = lagi::platform::input::launcher_stick_direction();
        if (pressed & 0x0010u) direction = 0;      // D-pad up
        else if (pressed & 0x0080u) direction = 1; // D-pad right
        else if (pressed & 0x0020u) direction = 2; // D-pad down
        else if (pressed & 0x0040u) direction = 3; // D-pad left
        if (direction >= 0 && discAvailable[direction])
            selection = direction;
        if (selection >= 0 && (pressed & 0x0002u)) {
            selectedDiscMounted = lagi::disc::mount_disc(selection + 1);
            if (!selectedDiscMounted) {
                discAvailable[selection] = false;
                selection = -1;
                for (int i = 0; i < 4; ++i)
                    if (discAvailable[i]) { selection = i; break; }
            }
        }
        if (!selectedDiscMounted) {
            lagi::platform::renderer::launcher_present(discAvailable, selection);
            lagi::platform::end_frame();
        }
    }

    if (!selectedDiscMounted) {
        lagi::platform::shutdown();
        sceKernelExitProcess(0);
        return 0;
    }
    lagi::platform::logging::writef(
        "[DiscSelector] mounted Disc %d: %s\n",
        lagi::disc::selected_disc(), lagi::disc::image_path());

    // A 30-vblank fade (~0.5 seconds at 60 Hz) masks the launcher-to-
    // native-loading-screen handoff. No Azel simulation occurs during it.
    lagi::platform::logging::writef("[DiscSelector] fade start (30 vblanks)\n");
    for (int frame = 0; frame <= 30; ++frame) {
        lagi::platform::renderer::launcher_fade(frame, 30);
        lagi::platform::end_frame();
    }
    lagi::platform::logging::writef("[DiscSelector] fade complete; initialize Azel\n");

    if (!lagi::azel::runtime_init()) {
        lagi::platform::logging::writef(
            "Azel runtime init failed.\n");
        lagi::platform::shutdown();
        sceKernelExitProcess(2);
        return 2;
    }

    lagi::platform::logging::writef(
        "Azel runtime: native boot task graph active\n");

    while (lagi::platform::running()) {
        lagi::platform::begin_frame();
        lagi::azel::runtime_frame();
        lagi::platform::end_frame();
    }

    lagi::platform::shutdown();
    sceKernelExitProcess(0);
    return 0;
}
