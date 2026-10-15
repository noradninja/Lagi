#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "commonOverlay.h"
#include "kernel/menuCursor.h"

void switchVdp2MenuTileToHightlighted(u32 offset,u32 width,u32 height,u32 palette);

namespace {
bool readSelectionData(LagiAzelMapSelectionData& data)
{
    return gCommonFile && gCommonFile->m_base==0x200000 &&
        lagiAzelReadMapSelectionData(gCommonFile->m_data,gCommonFile->m_dataSize,data);
}

struct LagiMapSelectionCursorTask : s_workAreaTemplateWithArg<LagiMapSelectionCursorTask,
    std::shared_ptr<LagiAzelMapCursorState>>
{
    std::shared_ptr<LagiAzelMapCursorState> state;
    LagiAzelMapSelectionData data{};
    // Reuse native Update/Draw. The embedded work area is not separately
    // scheduled; its callbacks only consume these cursor fields.
    s_MenuCursorWorkArea native;

    static void Init(LagiMapSelectionCursorTask* self,std::shared_ptr<LagiAzelMapCursorState> state)
    {
        self->state=std::move(state);
        if(!self->state) {self->getTask()->markFinished();return;}
        self->state->ready=false;self->state->finished=false;
        if(!readSelectionData(self->data)) {self->getTask()->markFinished();return;}
        self->native.m0_selectedEntry=-1;
        self->native.m4=&graphicEngineStatus.m40BC_layersConfig[self->data.layerIndex];
        self->native.m8=self->data.positions.data();
        self->native.mC=0;
        self->state->ready=true;
    }

    static void Update(LagiMapSelectionCursorTask* self)
    {
        if(self->state && self->state->ready)
            s_MenuCursorWorkArea::menuCursorTaskUpdate(&self->native);
    }

    static void Draw(LagiMapSelectionCursorTask* self)
    {
        if(!self->state || !self->state->ready) return;
        const auto selected=self->state->selected;
        self->native.m0_selectedEntry=selected>=0 && selected<2 ? selected : -1;
        s_MenuCursorWorkArea::menuCursorTaskDraw(&self->native);
    }

    static void Delete(LagiMapSelectionCursorTask* self)
    {
        if(self->state) {self->state->ready=false;self->state->finished=true;}
    }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition={&Init,&Update,&Draw,&Delete};
        return &definition;
    }
};
} // namespace

s_workArea* lagiAzelCreateMapSelectionCursor(s_workArea* parent,
    std::shared_ptr<LagiAzelMapCursorState> state)
{
    if(!parent || !state || (state->ready && !state->finished)) return nullptr;
    return createSubTaskWithArg<LagiMapSelectionCursorTask>(parent,std::move(state));
}

bool lagiAzelHighlightMapSelection(int slot,bool selected)
{
    if(slot<0 || slot>=2) return false;
    LagiAzelMapSelectionData data;
    if(!readSelectionData(data)) return false;
    // Native06016D88/06017762. Retain the shared routine's treatment of
    // empty palette words and its palette-field encoding.
    switchVdp2MenuTileToHightlighted(data.highlightVram[slot],3,3,selected?0x620:0x660);
    return true;
}
