#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/azel_map_menu.h"
#include "lagi/azel_books_menu.h"
#include "audio/systemSounds.h"
#include "commonOverlay.h"

namespace {

static void emitMapSprite(u16 control,u16 source,u16 size,u16 palette,
    const std::array<std::array<s16,2>,4>& corners)
{
    auto& context=graphicEngineStatus.m14_vdp1Context[0];
    if(context.m0_currentVdp1WriteEA==mainContextVdp1[0].end() ||
        !context.m20_pCurrentVdp1Packet) return;
    auto& command=*context.m0_currentVdp1WriteEA;
    command={};
    command.m0_CMDCTRL=control;
    command.m4_CMDPMOD=0x80;
    command.m6_CMDCOLR=palette;
    command.m8_CMDSRCA=source;
    command.mA_CMDSIZE=size;
    command.mC_CMDXA=corners[0][0];command.mE_CMDYA=corners[0][1];
    command.m10_CMDXB=corners[1][0];command.m12_CMDYB=corners[1][1];
    command.m14_CMDXC=corners[2][0];command.m16_CMDYC=corners[2][1];
    command.m18_CMDXD=corners[3][0];command.m1A_CMDYD=corners[3][1];
    context.m20_pCurrentVdp1Packet->m4_bucketTypes=0;
    context.m20_pCurrentVdp1Packet->m6_vdp1EA=&command;
    ++context.m20_pCurrentVdp1Packet;
    ++context.m0_currentVdp1WriteEA;
    ++context.m1C;
    ++context.mC;
}

static void drawMapPlayer(const LagiAzelMapContext& player)
{
    const auto& map=graphicEngineStatus.m40BC_layersConfig[2];
    std::array<std::int16_t,2> screen;
    if(!lagiAzelMapMarkerScreenPosition(player.x,player.y,map.scrollX,map.scrollY,
        graphicEngineStatus.m40BC_layersConfig[0].scrollY,screen)) return;
    // MAIN 06016FF8..06017142. Quad dimensions and command are native,
    // including the marker's forward displacement from the player position.
    const unsigned angle=((0u-static_cast<u32>(player.heading))>>16)&0xFFF;
    const s32 sine=getSin(angle).asS32(),cosine=getCos(angle).asS32();
    const s32 x=screen[0]-0xB0,y=screen[1]-0x70;
    const auto point=[&](s32 dx,s32 dy) {
        return std::array<s16,2>{static_cast<s16>(x+(dx>>16)),
            static_cast<s16>(y+(dy>>16))};
    };
    const std::array<std::array<s16,2>,4> corners={
        point(8*cosine+24*sine,8*sine-24*cosine),
        point(-8*cosine+24*sine,-8*sine-24*cosine),
        point(-8*cosine-7*sine,-8*sine-7*cosine),
        point(8*cosine-7*sine,8*sine-7*cosine)};
    emitMapSprite(0x1002,0x23D0,0x0220,0x0700,corners);
}

struct LagiFieldMapViewTask : s_workAreaTemplateWithArg<LagiFieldMapViewTask,const LagiAzelMapViewSource*>
{
    LagiAzelMapViewSource source{};
    s32 centerX=0,centerY=0,previousX=0,previousY=0;
    s32 phase=0,count=0;
    std::array<s16,2> savedScroll{};
    bool playerVisible=false;

    static void Init(LagiFieldMapViewTask* self,const LagiAzelMapViewSource* input)
    {
        if(!input) {self->getTask()->markFinished();return;}
        self->source=*input;
        if(self->source.status) *self->source.status={};
        const auto& descriptor=self->source.context.descriptor.presentation;
        const int minX=static_cast<std::int8_t>(descriptor[0])+2;
        const int minY=static_cast<std::int8_t>(descriptor[1])+2;
        const int maxX=static_cast<std::int8_t>(descriptor[2])-3;
        const int maxY=static_cast<std::int8_t>(descriptor[3])-3;
        if(!input->resourcesReady || !input->status || minX>maxX || minY>maxY ||
            input->context.subfield<0 || !lagiAzelMapContextIsCurrent(input->context)) {
            self->getTask()->markFinished();return;
        }
        self->centerX=std::clamp(intDivide(32,input->context.x).asS32(),minX,maxX);
        self->centerY=std::clamp(intDivide(32,input->context.y).asS32(),minY,maxY);
        self->previousX=self->centerX;self->previousY=self->centerY;
    }

    void writeScroll(std::array<s16,2> value)
    {
        auto& layer=graphicEngineStatus.m40BC_layersConfig[2];
        layer.scrollX=value[0];layer.scrollY=value[1];
    }

    void setRatio(unsigned ratio)
    {
        auto* regs=vdp2Controls.m4_pendingVdp2Regs;
        if(!regs) return;
        regs->m10A_CCRNB=(regs->m10A_CCRNB&0xFFE0)|(ratio&31);
        vdp2Controls.m_isDirty=1;
    }

    bool setup()
    {
        if(!lagiAzelPresentMapContextPatterns(source.context,source.patterns.data(),0x800,
            source.patterns.data()+0x800,0x800)) return false;
        savedScroll=lagiAzelSetMapViewCenter(static_cast<s16>(centerX),static_cast<s16>(centerY));
        const auto& backing=graphicEngineStatus.m40BC_layersConfig[1];
        lagiAzelSetupMapViewWindow(backing.scrollX,backing.scrollY);
        source.status->setupSucceeded=true;
        return true;
    }

    void input()
    {
        const auto& buttons=graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
        if(buttons.m8_newButtonDown&1) {
            playSystemSoundEffect(1);phase=3;return;
        }
        const auto& bounds=source.context.descriptor.presentation;
        // Shared native clamped-index routines 06015D30/06015DA4, also
        // consumed by Books. Preserve the original global feedback latches.
        auto axis=[&](s32& index,int low,int high,bool vertical,u8& nativeLatch) {
            bool latch=nativeLatch!=0;
            const auto step=lagiAzelBooksStepIndex(index,low,high,
                buttons.mC_newButtonDown2,buttons.m6_buttonDown,vertical,latch);
            nativeLatch=latch?1:0;
            if(step.sound>=0) playSystemSoundEffect(step.sound);
            return step.accepted;
        };
        bool moved=axis(centerX,static_cast<std::int8_t>(bounds[0])+2,
            static_cast<std::int8_t>(bounds[2])-3,false,graphicEngineStatus.m40AC.m6);
        moved=axis(centerY,static_cast<std::int8_t>(bounds[1])+2,
            static_cast<std::int8_t>(bounds[3])-3,true,graphicEngineStatus.m40AC.m7)||moved;
        writeScroll(savedScroll);
        if(moved) phase=5;
    }

    void drawDecorations()
    {
        const auto& bounds=source.context.descriptor.presentation;
        const bool enabled[4]={centerX>static_cast<std::int8_t>(bounds[0])+2,
            centerX<static_cast<std::int8_t>(bounds[2])-3,
            centerY>static_cast<std::int8_t>(bounds[1])+2,
            centerY<static_cast<std::int8_t>(bounds[3])-3};
        if(gCommonFile) for(unsigned i=0;i<4;++i) if(enabled[i]) {
            // Native COMMON 20E8BC, four ten-byte sprite records.
            const auto definition=gCommonFile->getSaturnPtr(0x20E8BC)+i*10;
            std::array<std::array<s16,2>,4> corners{};
            corners[0]={static_cast<s16>(readSaturnS16(definition+4)-0xB0),
                static_cast<s16>(readSaturnS16(definition+6)-0x70)};
            emitMapSprite(0x1000|readSaturnU16(definition+8),readSaturnU16(definition),
                readSaturnU16(definition+2),0x760,corners);
        }
        const auto& map=graphicEngineStatus.m40BC_layersConfig[2];
        lagiAzelDrawMapContextObjectMarkers(source.context,
            map.scrollX,map.scrollY,graphicEngineStatus.m40BC_layersConfig[0].scrollY);
    }

    static void Draw(LagiFieldMapViewTask* self)
    {
        switch(self->phase) {
        case 0:
            if(self->setup()) {self->count=16;self->phase=1;}
            break;
        case 1:
            self->setRatio(unsigned(--self->count)*2);
            if(self->count>0) break;
            self->playerVisible=true;self->phase=2;
            [[fallthrough]];
        case 2:
            self->input();self->drawDecorations();break;
        case 3:
            self->count=16;self->playerVisible=false;self->phase=4;
            [[fallthrough]];
        case 4:
            self->setRatio(unsigned(31-2*--self->count));
            if(self->count<=0) self->getTask()->markFinished();
            break;
        case 5:
            self->count=1;self->phase=6;
            [[fallthrough]];
        case 6:
            self->writeScroll({static_cast<s16>(self->savedScroll[0]+
                intDivide(8,(self->centerX-self->previousX)*32*self->count).asS32()),
                static_cast<s16>(self->savedScroll[1]+
                intDivide(8,(self->centerY-self->previousY)*32*self->count).asS32())});
            self->drawDecorations();
            if(++self->count<8) break;
            self->phase=7;
            [[fallthrough]];
        case 7:
            self->savedScroll=lagiAzelSetMapViewCenter(static_cast<s16>(self->centerX),static_cast<s16>(self->centerY));
            self->previousX=self->centerX;self->previousY=self->centerY;self->phase=2;
            break;
        default: break;
        }
        // Original marker is a subtask drawn after its parent, including the
        // final pan snap in phase7. It is hidden during opening/closing.
        if(self->playerVisible) drawMapPlayer(self->source.context);
    }

    static void Delete(LagiFieldMapViewTask* self)
    {
        lagiAzelRestoreMapViewPriorities();
        if(self->source.status) self->source.status->finished=true;
    }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition={&Init,nullptr,&Draw,&Delete};
        return &definition;
    }
};
} // namespace

s_workArea* lagiAzelCreateFieldMapView(s_workArea* parent,const LagiAzelMapViewSource& source)
{
    if(!parent || !source.resourcesReady || !source.status) return nullptr;
    return createSubTaskWithArg<LagiFieldMapViewTask>(parent,&source);
}
