#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "lagi/lagi_scene_bridge.h"
#include "commonOverlay.h"
#include "kernel/dialogTask.h"
#include "VDP2.h"
#include "audio/systemSounds.h"

namespace {
struct LagiMapWorldConfirmationTask : s_workAreaTemplateWithArg<LagiMapWorldConfirmationTask,
    std::shared_ptr<LagiAzelMapConfirmationState>>
{
    std::shared_ptr<LagiAzelMapConfirmationState> state;
    int phase=0,selected=0,cursorFrame=0,cursorTicks=0;
    const char* text=nullptr;
    bool initialized=false;
    // MAIN06017736 passes x8,y39 to0601C710. Width10,height4 are fixed there.
    static constexpr int x=8,y=0x39,width=0x10,height=4;

    static void Init(LagiMapWorldConfirmationTask* self,
        std::shared_ptr<LagiAzelMapConfirmationState> state)
    {
        self->state=std::move(state);
        if(!self->state) {self->getTask()->markFinished();return;}
        self->state->ready=false;self->state->finished=false;self->state->running=true;self->state->result=0;
        if(!gCommonFile || !gCommonFile->m_data || gCommonFile->m_base!=0x200000 ||
           gCommonFile->m_dataSize<0x1004C) {self->getTask()->markFinished();return;}
        const auto* p=gCommonFile->m_data+0x10048;
        const std::uint32_t address=(std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|
            (std::uint32_t(p[2])<<8)|p[3];
        if(address<0x200000 || address-0x200000>=gCommonFile->m_dataSize) {
            self->getTask()->markFinished();return;
        }
        const auto offset=address-0x200000;
        self->text=reinterpret_cast<const char*>(gCommonFile->m_data+offset);
        if(!std::memchr(self->text,0,gCommonFile->m_dataSize-offset)) {
            self->getTask()->markFinished();return;
        }
        self->state->ready=true;
    }

    static void Update(LagiMapWorldConfirmationTask* self)
    {
        if(!self->state || !self->state->ready) return;
        switch(self->phase) {
        case 0:
        case 1:
            drawBlueBox(x,y,width,height,0x1000);
            setupVDP2StringRendering(x+2,y+1,width-4,height-2);
            vdp2StringContext.m0=0;
            vdp2StringContext.m4_cursorX=vdp2StringContext.mC_X;
            vdp2StringContext.m8_cursorY=vdp2StringContext.m10_Y;
            drawObjectName(self->text);
            self->initialized=true;
            playSystemSoundEffect(3);self->phase=2;return;
        case 2: {
            const auto input=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown;
            if(input&0x80) {
                if(!self->selected) {self->selected=1;playSystemSoundEffect(2);}
            } else if(input&0x40) {
                if(self->selected==1) {self->selected=0;playSystemSoundEffect(2);}
            }
            if(input&6) {
                self->state->result=self->selected^1;
                playSystemSoundEffect(0);self->phase=3;
            } else if(input&1) {
                self->state->result=0;playSystemSoundEffect(1);self->phase=3;
            }
            return;
        }
        //0601C5B6 clears task-header Draw at workArea-24+C, not result+C.
        case 3: self->phase=4;return;
        case 4: self->getTask()->markFinished();return;
        }
    }

    static void Draw(LagiMapWorldConfirmationTask* self)
    {
        if(!self->state || !self->state->ready || self->phase>=4) return;
        std::int16_t localX=176,localY=112;
        lagi::scene_bridge::capture_local_coordinates(localX,localY);
        // Existing native packet helper subtracts176/112. Bias its input to
        // reproduce0601C5D8's actual current VDP1 local origin instead.
        drawMultiChoiceVdp1Cursor((x+2+self->selected*6)*8-localX+176,
            (y+1)*8-localY+112,
            gCommonFile->getSaturnPtr(0x20FFE0+self->cursorFrame*10),0x7F0);
        self->cursorTicks+=vblankData.m14_numVsyncPerFrame;
        if(self->cursorTicks>=2) {
            self->cursorFrame=(self->cursorFrame+1)&7;self->cursorTicks=0;
        }
    }

    static void Delete(LagiMapWorldConfirmationTask* self)
    {
        if(self->initialized) {
            setupVDP2StringRendering(x+2,y+1,width-4,height-2);
            clearVdp2TextArea();clearBlueBox(x,y,width,height);
        }
        if(self->state) {self->state->ready=false;self->state->finished=true;self->state->running=false;}
    }
    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition={&Init,&Update,&Draw,&Delete};
        return &definition;
    }
};
} // namespace

s_workArea* lagiAzelCreateMapWorldConfirmation(s_workArea* parent,
    std::shared_ptr<LagiAzelMapConfirmationState> state)
{
    if(!parent || !state || state->running) return nullptr;
    return createSubTaskWithArg<LagiMapWorldConfirmationTask>(parent,std::move(state));
}
