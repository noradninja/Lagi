#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "kernel/moduleManager.h"

// Native mode 6 is WORLD.PRG, not return-to-title. Keep the selected module
// alive while its unported display/travel tasks remain unavailable. Do not run
// the placeholder map initializer: its save-task call resets the map allocator.
struct LagiWorldBoundary : s_workAreaTemplate<LagiWorldBoundary> {
    std::vector<std::uint8_t> program;
    static void Init(LagiWorldBoundary* task) {
        const bool loaded=lagi::disc::read_file("WORLD.PRG",task->program);
        lagi::platform::logging::writef(
            "[WorldMapStartup] status=%02X mode=%d WORLD.PRG loaded=%u bytes=%u; "
            "display/travel unported, holding native boundary (not a playable map)\n",
            unsigned(gGameStatus.m4_gameStatus),int(gGameStatus.m0_gameMode),
            loaded?1u:0u,unsigned(task->program.size()));
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition={Init,nullptr,nullptr,nullptr};
        return &definition;
    }
};

p_workArea lagiLoadNativeWorldBoundary(p_workArea parent,s32)
{
    // Return a real child, never the manager itself as upstream's unfinished
    // loadWorldOverlay currently does. No setNextGameStatus/fallback is issued.
    return createSubTask<LagiWorldBoundary>(parent);
}
