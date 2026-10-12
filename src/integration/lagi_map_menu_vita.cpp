#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "audio/systemSounds.h"
#include "kernel/textDisplay.h"

// Azel's main menu owns this setup. Restore it when the Map child exits,
// so the original status-menu task can resume without a replacement UI.
void initVdp2ForStatusMenu();

namespace {

struct LagiFieldMapMenuTask : s_workAreaTemplate<LagiFieldMapMenuTask>
{
    bool mapOwned = false;

    static void Init(LagiFieldMapMenuTask* self)
    {
        self->mapOwned = lagiAzelCurrentFieldMapOwned();
        if (!lagiAzelSetupMapEntry(false)) {
            self->getTask()->markFinished();
            return;
        }

        // Map entry art and text stay in Azel's native VDP2 memory.
        // NBG2 field-map viewport submission is the next bring-up stage.
        setupVDP2StringRendering(0, 0x22, 0x2C, 0x1C);
        clearVdp2TextArea();
    }

    static void Draw(LagiFieldMapMenuTask* self)
    {
        if (graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 1) {
            playSystemSoundEffect(1);
            self->getTask()->markFinished();
            return;
        }

        // Retain native text output until the VDP2 map viewport is active.
        // Do not synthesize a map or use the separate world-map overlay.
        vdp2StringContext.m0 = 0;
        vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X + 3;
        vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + 3;
        drawObjectName("FIELD MAP");
        vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X + 3;
        vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y + 6;
        drawObjectName(self->mapOwned ? "Map acquired" : "Map not acquired");
    }

    static void Delete(LagiFieldMapMenuTask*)
    {
        lagiAzelRestoreMapViewPriorities();
        initVdp2ForStatusMenu();
    }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition = {&Init, nullptr, &Draw, &Delete};
        return &definition;
    }
};

} // namespace

// Override the pinned Azel assertion at the existing native dispatch site.
p_workArea createMapTask(p_workArea parent)
{
    return createSubTask<LagiFieldMapMenuTask>(parent);
}
