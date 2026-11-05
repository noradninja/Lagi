#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/town_yes_no.h"
#include "kernel/dialogTask.h"
#include "commonOverlay.h"
#include "audio/systemSounds.h"

namespace {
// MAIN0601C42C uses the original COMMON string, with both choices on one row.
void drawTownYesNoBox(s_multiChoiceTask* task)
{
    drawBlueBox(task->m14_x, task->m16_y, task->m1A_width, task->m1C_height, 0x1000);
    setupVDP2StringRendering(task->m14_x + 2, task->m16_y + 1,
                            task->m1A_width - 4, task->m1C_height - 2);
    vdp2StringContext.m0 = 0;
    vdp2StringContext.m4_cursorX = vdp2StringContext.mC_X;
    vdp2StringContext.m8_cursorY = vdp2StringContext.m10_Y;
    drawObjectName(readSaturnString(task->m24_strings).c_str());
}

// COMMON0021006C binds MAIN0601C4F0/0601C5D8/0601C6B0.
void updateTownYesNo(s_multiChoiceTask* task)
{
    switch (task->m0_Status) {
    case 0:
        ++task->m0_Status;
        [[fallthrough]];
    case 1:
        drawTownYesNoBox(task);
        playSystemSoundEffect(3);
        ++task->m0_Status;
        return;
    case 2: {
        const auto buttons = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown;
        if ((buttons & 0x80) && task->m5_selectedEntry == 0) {
            task->m5_selectedEntry = 1;
            playSystemSoundEffect(2);
        } else if (!(buttons & 0x80) && (buttons & 0x40) && task->m5_selectedEntry == 1) {
            task->m5_selectedEntry = 0;
            playSystemSoundEffect(2);
        }
        if (buttons & 6) {
            *task->mC_result = task->m5_selectedEntry ^ 1;
            playSystemSoundEffect(0);
            ++task->m0_Status;
        } else if (buttons & 1) {
            *task->mC_result = 0;
            playSystemSoundEffect(1);
            ++task->m0_Status;
        }
        return;
    }
    case 3:
        task->mC_result = nullptr;
        ++task->m0_Status;
        return;
    case 4:
        task->getTask()->markFinished();
        return;
    }
}

void drawTownYesNoCursor(s_multiChoiceTask* task)
{
    if (task->m5_selectedEntry == 0 || task->m5_selectedEntry == 1) {
        drawMultiChoiceVdp1Cursor((task->m14_x + 2 + task->m5_selectedEntry * 6) * 8,
                                 (task->m16_y + 1) * 8,
                                 gCommonFile->getSaturnPtr(0x20FFE0 + task->m7 * 10), 0x7F0);
    }
    task->m8 = static_cast<s8>(task->m8 + vblankData.m14_numVsyncPerFrame);
    if (task->m8 >= 2) {
        task->m7 = static_cast<s8>(task->m7 + 1);
        if (static_cast<unsigned>(task->m7) >= 8) task->m7 = 0;
        task->m8 = 0;
    }
}
}

void lagiStartTownYesNoDialog(s_workArea* parent, s_multiChoiceTask** output, s32* result)
{
    static const s_multiChoiceTask::TypedTaskDefinition definition = {
        nullptr, updateTownYesNo, drawTownYesNoCursor, &s_multiChoiceTask::Delete,
    };
    auto* task = createSubTask<s_multiChoiceTask>(parent, &definition);
    task->m0_Status = 0;
    task->m1 = 0;
    task->m2_defaultResult = 0;
    task->m3_quantity = 0;
    task->m4_quantityMax = 0;
    task->m5_selectedEntry = 0;
    task->m6_numEntries = 0;
    task->m7 = 0;
    task->m8 = 0;
    task->mC_result = result;
    task->m10_outputTask = output;
    task->m14_x = 24;
    task->m16_y = 20;
    task->m1A_width = 16;
    task->m1C_height = 4;
    task->m20 = 0;
    task->m24_strings = readSaturnEA(gCommonFile->getSaturnPtr(0x210048));
    task->m28_colors = nullptr;
    if (output) *output = task;
}
