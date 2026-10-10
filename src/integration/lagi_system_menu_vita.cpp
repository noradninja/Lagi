#include "lagi/lagi_azel_upstream_prelude.h"

#include "lagi/azel_save_platform.h"
#include "lagi/platform.h"
#include "audio/systemSounds.h"
#include "kernel/loadSavegameScreen.h"
#include "kernel/moduleManager.h"

#include <cstdint>

namespace {

enum class ChildMode : std::uint8_t {
    None,
    Save,
    Load,
};

struct LagiSystemMenuTask
    : public s_workAreaTemplate<LagiSystemMenuTask>
{
    static TypedTaskDefinition* getTypedTaskDefinition()
    {
        static TypedTaskDefinition definition = {
            &LagiSystemMenuTask::Init,
            nullptr,
            &LagiSystemMenuTask::Draw,
            nullptr,
        };
        return &definition;
    }

    static void render(LagiSystemMenuTask* self)
    {
        setupVDP2StringRendering(0, 34, 44, 28);
        clearVdp2TextArea();
        static const char* entries[] = {
            "SAVE GAME",
            "LOAD GAME",
            "RETURN",
        };
        for (int index = 0; index < 3; ++index) {
            vdp2PrintStatus.m10_palette =
                index == self->selection ? 0xC000 : 0x9000;
            vdp2DebugPrintSetPosition(16, 40 + index * 3);
            drawLineLargeFont(entries[index]);
        }
    }

    static void Init(LagiSystemMenuTask* self)
    {
        self->selection = 0;
        self->child = nullptr;
        self->childMode = ChildMode::None;
        lagi::platform::logging::writef("[SaveMenu] opened\n");
        render(self);
    }

    static void prepareSaveLocation()
    {
        gSaveGameStatus.m8_gameMode =
            static_cast<u8>(gGameStatus.m4_gameStatus);
        gSaveGameStatus.m9_fieldIndex = 0;
        gSaveGameStatus.mA_subFieldIndex = 0;
        gSaveGameStatus.mB_entryPointIndex = 0;

        if (gGameStatus.m0_gameMode == 3) {
            if (s_fieldTaskWorkArea* field = getFieldTaskPtr()) {
                gSaveGameStatus.m9_fieldIndex =
                    static_cast<u8>(field->m2C_currentFieldIndex);
                gSaveGameStatus.mA_subFieldIndex =
                    static_cast<u8>(field->m2E_currentSubFieldIndex);
                gSaveGameStatus.mB_entryPointIndex =
                    static_cast<s8>(field->m30_fieldEntryPoint);
            }
        }
    }

    static int loadedTargetStatus()
    {
        const int savedDisc = mainGameState.readPackedBits(0xD4, 2);
        if (savedDisc != static_cast<int>(azelCdNumber))
            return savedDisc + 0x4B;
        if (gSaveGameStatus.m8_gameMode != 0)
            return gSaveGameStatus.m8_gameMode;
        return azelCdNumber == 0 ? 1 : 0x4F;
    }

    static void Draw(LagiSystemMenuTask* self)
    {
        if (self->child) {
            if (!self->child->getTask()->isFinished())
                return;
            self->child = nullptr;
            if (self->childMode == ChildMode::Load &&
                gSaveGameStatus.m4_version == 0x10000u) {
                const int targetStatus = loadedTargetStatus();
                lagi::platform::logging::writef(
                    "[SaveMenu] loaded targetStatus=%d field=%u subfield=%u entry=%d\n",
                    targetStatus,
                    static_cast<unsigned>(gSaveGameStatus.m9_fieldIndex),
                    static_cast<unsigned>(gSaveGameStatus.mA_subFieldIndex),
                    static_cast<int>(gSaveGameStatus.mB_entryPointIndex));
                lagiAzelSaveMarkInGameLoad();
                setNextGameStatus(targetStatus);
                self->getTask()->markFinished();
                return;
            }
            self->childMode = ChildMode::None;
            render(self);
            return;
        }

        const auto& input =
            graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
        if ((input.m8_newButtonDown & 1u) != 0u) {
            playSystemSoundEffect(1);
            self->getTask()->markFinished();
            return;
        }
        if ((input.mC_newButtonDown2 & 0x30u) != 0u) {
            if ((input.mC_newButtonDown2 & 0x10u) != 0u)
                self->selection = (self->selection + 2) % 3;
            else
                self->selection = (self->selection + 1) % 3;
            playSystemSoundEffect(10);
            render(self);
            return;
        }
        if ((input.m8_newButtonDown & 6u) == 0u)
            return;

        playSystemSoundEffect(0);
        if (self->selection == 0) {
            prepareSaveLocation();
            lagi::platform::logging::writef(
                "[SaveMenu] launch save status=%u field=%u subfield=%u entry=%d\n",
                static_cast<unsigned>(gSaveGameStatus.m8_gameMode),
                static_cast<unsigned>(gSaveGameStatus.m9_fieldIndex),
                static_cast<unsigned>(gSaveGameStatus.mA_subFieldIndex),
                static_cast<int>(gSaveGameStatus.mB_entryPointIndex));
            self->childMode = ChildMode::Save;
            self->child = createSaveTask(self);
        } else if (self->selection == 1) {
            lagi::platform::logging::writef("[SaveMenu] launch load\n");
            gSaveGameStatus.m4_version = 0;
            self->childMode = ChildMode::Load;
            self->child = createLoadTask(self);
        } else {
            self->getTask()->markFinished();
        }
    }

    int selection = 0;
    p_workArea child = nullptr;
    ChildMode childMode = ChildMode::None;
};

} // namespace

p_workArea createSystemMenuTask(p_workArea parent)
{
    return createSubTask<LagiSystemMenuTask>(parent);
}
