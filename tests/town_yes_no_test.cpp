#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using u8 = std::uint8_t;
struct sSaturnPtr {
    unsigned address = 0;
    sSaturnPtr operator-(unsigned count) const { return {address - count}; }
};
struct s_workArea {};
using p_workArea = s_workArea*;
struct Task { bool finished = false; void markFinished() { finished = true; } };
template<class T> struct s_workAreaTemplate : s_workArea {
    struct TypedTaskDefinition {
        void (*init)(T*);
        void (*update)(T*);
        void (*draw)(T*);
        void (*destroy)(T*);
    };
    Task task;
    Task* getTask() { return &task; }
};
int textClears = 0, boxClears = 0;
std::array<int, 5> box{};
std::array<int, 4> textArea{};
std::vector<int> sounds;
struct { int m0 = 0, m4_cursorX = 0, m8_cursorY = 0, mC_X = 0, m10_Y = 0; } vdp2StringContext;
void setupVDP2StringRendering(int x, int y, int width, int height) {
    textArea = {x, y, width, height};
    vdp2StringContext.mC_X = x;
    vdp2StringContext.m10_Y = y;
}
void clearVdp2TextArea() { ++textClears; }
void clearBlueBox(int, int, int, int) { ++boxClears; }
void drawBlueBox(int x, int y, int width, int height, int flags) { box = {x, y, width, height, flags}; }
void playSystemSoundEffect(int sound) { sounds.push_back(sound); }

#include "kernel/dialogTask.h"

const s_multiChoiceTask::TypedTaskDefinition* dialogDefinition = nullptr;
p_workArea dialogParent = nullptr;
int creations = 0;
template<class T> T* createSubTask(p_workArea parent, const typename T::TypedTaskDefinition* definition) {
    dialogDefinition = definition;
    dialogParent = parent;
    ++creations;
    return new T;
}
struct CommonFile {
    sSaturnPtr getSaturnPtr(unsigned address) { return {address}; }
} commonFile;
CommonFile* gCommonFile = &commonFile;
sSaturnPtr readSaturnEA(sSaturnPtr pointer) { assert(pointer.address == 0x210048); return {0x20F000}; }
std::string readSaturnString(sSaturnPtr pointer) { assert(pointer.address == 0x20F000); return "native prompt"; }
void drawObjectName(const char* text) { assert(std::string(text) == "native prompt"); }
std::array<int, 4> cursor{};
void drawMultiChoiceVdp1Cursor(s32 x, s32 y, sSaturnPtr sprite, s32 color) {
    cursor = {x, y, int(sprite.address), color};
}
struct { struct { struct { struct { unsigned m8_newButtonDown = 0; } m0_current; } m0_inputDevices[1]; } m4514; } graphicEngineStatus;
struct { int m14_numVsyncPerFrame = 1; } vblankData;
struct DisplayTask : s_workAreaTemplate<DisplayTask> {};
struct sNpcData {
    s_multiChoiceTask* m170_multiChoiceTask = nullptr;
    DisplayTask* m16C_displayStringTask = nullptr;
    int m118_currentResult = 99, mF0 = 1;
};
s_workArea parent;
p_workArea currentResTask = &parent;

#include "production.inc"

sSaturnPtr runYesNoOpcode(sNpcData* r13_pThis, sSaturnPtr r14) {
    switch (38) {
#include "opcode.inc"
    }
    assert(false);
    return {};
}

int main() {
    sNpcData context;
    DisplayTask display;
    context.m16C_displayStringTask = &display;
    int before = creations;
    assert(runYesNoOpcode(&context, {0x6058611}).address == 0x6058610);
    assert(display.task.finished && creations == before);
    context.m16C_displayStringTask = nullptr;
    assert(runYesNoOpcode(&context, {0x6058611}).address == 0x6058610);
    auto* task = context.m170_multiChoiceTask;
    assert(task && dialogParent == &parent && task->m0_Status == 0);
    assert(task->m14_x == 24 && task->m16_y == 20 && task->m1A_width == 16 && task->m1C_height == 4);
    assert(task->mC_result == &context.m118_currentResult && task->m10_outputTask == &context.m170_multiChoiceTask);
    assert(task->m5_selectedEntry == 0);
    dialogDefinition->update(task);
    assert(task->m0_Status == 2 && sounds.back() == 3 && context.m118_currentResult == 99);
    assert((box == std::array<int, 5>{24, 20, 16, 4, 0x1000}));
    assert((textArea == std::array<int, 4>{26, 21, 12, 2}));
    assert(vdp2StringContext.m0 == 0 && vdp2StringContext.m4_cursorX == 26 && vdp2StringContext.m8_cursorY == 21);
    dialogDefinition->draw(task);
    assert((cursor == std::array<int, 4>{208, 168, 0x20FFE0, 0x7F0}));
    assert(task->m7 == 0 && task->m8 == 1);
    dialogDefinition->draw(task);
    assert(task->m7 == 1 && task->m8 == 0);
    task->m5_selectedEntry = 1;
    dialogDefinition->draw(task);
    assert(cursor[0] == 256 && cursor[1] == 168 && cursor[2] == 0x20FFEA);
    task->m7 = 7;
    task->m8 = 1;
    dialogDefinition->draw(task);
    assert(task->m7 == 0 && task->m8 == 0);

    for (unsigned selection = 0; selection < 2; ++selection) {
        for (unsigned buttons = 0; buttons < 256; ++buttons) {
            task->m0_Status = 2;
            task->m5_selectedEntry = selection;
            task->mC_result = &context.m118_currentResult;
            context.m118_currentResult = 99;
            graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown = buttons;
            dialogDefinition->update(task);
            unsigned expectedSelection = selection;
            if (buttons & 0x80) expectedSelection = 1;
            else if (buttons & 0x40) expectedSelection = 0;
            assert(task->m5_selectedEntry == expectedSelection);
            const bool complete = (buttons & 7) != 0;
            assert(task->m0_Status == (complete ? 3 : 2));
            assert(context.m118_currentResult == (buttons & 6 ? int(expectedSelection ^ 1) : buttons & 1 ? 0 : 99));
        }
    }
    for (int state = 0; state < 4; ++state) {
        task->m0_Status = state;
        assert(runYesNoOpcode(&context, {0x6058611}).address == 0x6058610);
    }
    task->m0_Status = 3;
    dialogDefinition->update(task);
    assert(task->m0_Status == 4 && task->mC_result == nullptr);
    assert(runYesNoOpcode(&context, {0x6058611}).address == 0x6058611);
    assert(context.mF0 == 0);
    dialogDefinition->update(task);
    assert(task->task.finished);
    before = boxClears;
    dialogDefinition->destroy(task);
    assert(context.m170_multiChoiceTask == nullptr && boxClears == before + 1 && textClears > 0);
    delete task;
    std::puts("PASS production Yes/No input, horizontal cursor, native text/layout, animation, opcode wait and cleanup");
}
