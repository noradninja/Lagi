#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "menu_dragon.h"
#include "audio/systemSounds.h"
#include "kernel/textDisplay.h"
#include "kernel/fade.h"

namespace {
// MAIN060179CE/06017A68/06017B74, with task-owned resource/status handles.
struct LagiMapParentTask : s_workAreaTemplateWithArg<LagiMapParentTask,const LagiAzelMapParentSource*>
{
    LagiAzelMapParentSource source{};
    LagiAzelMapLifecycle lifecycle{};
    LagiAzelMapResources resources;
    std::shared_ptr<LagiAzelMapCursorState> cursor;
    std::shared_ptr<LagiAzelMapViewStatus> view;
    std::shared_ptr<LagiAzelMapConfirmationState> confirmation;
    int selected=0;
    unsigned savedVsync=0;
    bool cadenceSaved=false;

    static void Init(LagiMapParentTask* self,const LagiAzelMapParentSource* input)
    {
        if(!input) {self->getTask()->markFinished();return;}
        self->source=*input;
        self->selected=input->availability.selected;
        self->lifecycle.standalone=input->standalone;
        self->lifecycle.phase=input->standalone?1:0;
        self->lifecycle.availabilitySlot0=input->availability.slots[0];
        if(!lagiAzelSetupMapEntry(input->standalone)) {
            self->getTask()->markFinished();return;
        }
        if(input->standalone) {
            self->savedVsync=vblankData.m14_numVsyncPerFrame;
            self->cadenceSaved=true;
            vblankData.m14_numVsyncPerFrame=1;
        } else {
            startVdp2LayerScroll(0,-10,0,16);
            startVdp2LayerScroll(1,0,-16,16);
        }
        for(int slot=0;slot<2;++slot)
            if(input->availability.slots[slot]==1)
                lagiAzelHighlightMapSelection(slot,slot==self->selected);
        if(input->availability.worldCover) lagiAzelCreateMapWorldUnavailableCover(self);
        if(input->hasContext) self->resources.load(input->context.descriptor);
        self->cursor=std::make_shared<LagiAzelMapCursorState>();
        if(!lagiAzelCreateMapSelectionCursor(self,self->cursor)) {
            self->getTask()->markFinished();return;
        }
        self->cursor->selected=-1;
    }

    void select()
    {
        lifecycle.mode=0;
        if(cursor) cursor->selected=selected;
        lagiAzelPresentMapMessage(selected,source.availability.slots);
    }

    void openField()
    {
        lifecycle.mode=1;
        if(cursor) cursor->selected=-1;
        view=std::make_shared<LagiAzelMapViewStatus>();
        LagiAzelMapViewSource input;
        input.context=source.context;
        input.patterns=resources.patterns();
        input.resourcesReady=source.hasContext && resources.ready();
        input.status=view;
        if(!lagiAzelCreateFieldMapView(this,input)) view->finished=true;
        lagiAzelClearMapMessage();
    }

    void openWorld()
    {
        lifecycle.mode=2;
        if(cursor) cursor->selected=-1;
        confirmation=std::make_shared<LagiAzelMapConfirmationState>();
        if(!lagiAzelCreateMapWorldConfirmation(this,confirmation)) confirmation->finished=true;
    }

    static void Draw(LagiMapParentTask* self)
    {
        const auto step=lagiAzelAdvanceMapLifecycle(self->lifecycle);
        switch(step.action) {
        case LagiAzelMapAction::SetupSelection: self->select();break;
        case LagiAzelMapAction::SetupStandaloneMode1: self->openField();break;
        case LagiAzelMapAction::InputSelection: {
            // Native06017776 reads the pressed word at input base+8.
            const auto input=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown;
            const auto result=lagiAzelMapSelect(self->selected,self->source.availability.slots,input);
            if(result.sound>=0) playSystemSoundEffect(result.sound);
            if(result.oldHighlight>=0) lagiAzelHighlightMapSelection(result.oldHighlight,false);
            if(result.newHighlight>=0) lagiAzelHighlightMapSelection(result.newHighlight,true);
            switch(result.action) {
            case LagiAzelMapSelectionResult::Cancel: self->lifecycle.phase=3;break;
            case LagiAzelMapSelectionResult::EnterMode1: self->openField();break;
            case LagiAzelMapSelectionResult::EnterMode2: self->openWorld();break;
            case LagiAzelMapSelectionResult::Moved: self->select();break;
            default: break;
            }
            break;
        }
        case LagiAzelMapAction::InputMode1:
            //060178B6 checks the task's finish bit, not Delete completion.
            if(!self->view || self->view->finished) {
                if(self->source.standalone) self->lifecycle.phase=3;
                else self->select();
            }
            break;
        case LagiAzelMapAction::InputMode2:
            // Confirmation's native output pointer is cleared by Delete.
            if(!self->confirmation || self->confirmation->finished) {
                if(self->confirmation) lagiAzelCommitMapWorldConfirmation(*self->confirmation);
                self->select();
            }
            break;
        case LagiAzelMapAction::CloseMenu:
            setupVDP2StringRendering(0,0x22,0x2C,0x1C);
            clearVdp2TextArea();
            startVdp2LayerScroll(0,10,0,16);
            startVdp2LayerScroll(1,0,16,16);
            break;
        case LagiAzelMapAction::CloseStandalone:
            fadePalette(&g_fadeControls.m0_fade0,0,0,1);
            fadePalette(&g_fadeControls.m24_fade1,0,0,1);
            break;
        default: break;
        }
        if(step.finished) self->getTask()->markFinished();
    }

    static void Delete(LagiMapParentTask* self)
    {
        // Task-tree teardown deletes children first. Keep authored graphics
        // and planes alive until the viewer can no longer reference them.
        self->resources.restore();
        if(self->cadenceSaved) vblankData.m14_numVsyncPerFrame=self->savedVsync;
    }
    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition={&Init,nullptr,&Draw,&Delete};
        return &definition;
    }
};
}

s_workArea* lagiAzelCreateMapParent(s_workArea* parent,const LagiAzelMapParentSource& source)
{
    const auto& a=source.availability;
    if(!parent || a.selected<0 || a.selected>1 || a.slots[0]<0 || a.slots[0]>1 ||
        a.slots[1]<-1 || a.slots[1]>1 || (a.selected==1 && a.slots[1]<0) ||
        a.slots[0]!=int(source.hasContext) || a.worldCover!=(a.slots[1]==-1)) return nullptr;
    return createSubTaskWithArg<LagiMapParentTask>(parent,&source);
}
