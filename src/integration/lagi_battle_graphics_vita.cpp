#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/battle_graphics.h"
#include "lagi/resident_video.h"
#include "lightSetup.h"
#include <new>
#include <cstring>

extern sVdpVar1* vdpVar2;

namespace {
struct BattleGraphics {
    lagi::resident_video::Snapshot resident;
    s_graphicEngineStatus_405C projection;
    sVdp2Controls controls;
    sFadeControls fade;
    std::array<sVdpVar1,14> transfers;
    sVdpVar1* head;
    sVdpVar1* tail;
    s_lightSetup light;
    u8 cram[0x1000];
    u8 vram[0x400];
    u8 optionalVram[0x800];
    u16 backColor, lineColor;
    s16 local[2], clip[4];
    int pending;
    bool includeFade, hasLocal, hasClip;
};

bool ownedTransfer(const sVdpVar1* pointer)
{
    if(!pointer)return true;
    for(const auto& entry:vdpVar1)if(pointer==&entry)return true;
    return false;
}
} // namespace

void* lagiCaptureBattleGraphics(bool includeFade)
{
    // Refuse an invalid owner graph before field state is changed. These are
    // pointers into permanent native owners, never into the snapshot itself.
    int pending=-1;
    for(int i=0;i<2;++i)
        if(vdp2Controls.m4_pendingVdp2Regs==&vdp2Controls.m20_registers[i])pending=i;
    if(pending<0 || !ownedTransfer(vdpVar2) || !ownedTransfer(vdpVar3))return nullptr;
    for(const auto& entry:vdpVar1)if(!ownedTransfer(entry.m10_nextTransfert))return nullptr;
    auto* saved=new(std::nothrow) BattleGraphics{};
    if(!saved)return nullptr;
    lagi::resident_video::capture(saved->resident);
    saved->projection=graphicEngineStatus.m405C;
    // Never retain pointers into recycled VDP1 command lists.
    saved->hasLocal=saved->projection.setLocalCoordinatesEA!=nullptr;
    if(saved->hasLocal){
        saved->local[0]=saved->projection.setLocalCoordinatesEA->mC_CMDXA;
        saved->local[1]=saved->projection.setLocalCoordinatesEA->mE_CMDYA;
    }
    saved->hasClip=saved->projection.setClippingCoordinatesEA!=nullptr;
    if(saved->hasClip){
        const auto& clip=*saved->projection.setClippingCoordinatesEA;
        saved->clip[0]=clip.mC_CMDXA;saved->clip[1]=clip.mE_CMDYA;
        saved->clip[2]=clip.m14_CMDXC;saved->clip[3]=clip.m16_CMDYC;
    }
    saved->projection.setLocalCoordinatesEA=nullptr;
    saved->projection.setClippingCoordinatesEA=nullptr;
    saved->controls=vdp2Controls;
    saved->controls.m4_pendingVdp2Regs=nullptr;
    saved->pending=pending;
    saved->includeFade=includeFade;
    if(includeFade)saved->fade=g_fadeControls;
    saved->transfers=vdpVar1;saved->head=vdpVar2;saved->tail=vdpVar3;
    saved->light=lightSetup;
    std::memcpy(saved->cram,getVdp2Cram(0),sizeof(saved->cram));
    std::memcpy(saved->vram,getVdp2Vram(0x3E000),sizeof(saved->vram));
    if(includeFade){
        std::memcpy(saved->optionalVram,getVdp2Vram(0x3C000),0x400);
        std::memcpy(saved->optionalVram+0x400,getVdp2Vram(0x3E000),0x400);
    }
    saved->backColor=getVdp2VramU16((vdp2Controls.m4_pendingVdp2Regs->mAC_BKTA&0x7FFFF)*2);
    saved->lineColor=getVdp2VramU16((vdp2Controls.m4_pendingVdp2Regs->mA8_LCTA&0x7FFFF)*2);
    return saved;
}

bool lagiPrepareBattleGraphicsRestore(void* backup)
{
    return !backup || lagi::resident_video::prepare(static_cast<BattleGraphics*>(backup)->resident);
}

bool lagiRestoreBattleGraphics(void*& backup)
{
    if(!backup)return true;
    auto* saved=static_cast<BattleGraphics*>(backup);
    // Native resource reload precedes the explicit VRAM/color-span restore.
    // No field resume or deletion occurs if staging failed or disc changed.
    if(!lagi::resident_video::restore(saved->resident))return false;
    auto* local=graphicEngineStatus.m405C.setLocalCoordinatesEA;
    auto* clip=graphicEngineStatus.m405C.setClippingCoordinatesEA;
    graphicEngineStatus.m405C=saved->projection;
    graphicEngineStatus.m405C.setLocalCoordinatesEA=local;
    graphicEngineStatus.m405C.setClippingCoordinatesEA=clip;
    if(local && saved->hasLocal){local->mC_CMDXA=saved->local[0];local->mE_CMDYA=saved->local[1];}
    if(clip && saved->hasClip){
        clip->mC_CMDXA=saved->clip[0];clip->mE_CMDYA=saved->clip[1];
        clip->m14_CMDXC=saved->clip[2];clip->m16_CMDYC=saved->clip[3];
    }
    vdp2Controls=saved->controls;
    vdp2Controls.m4_pendingVdp2Regs=&vdp2Controls.m20_registers[saved->pending];
    if(saved->includeFade)g_fadeControls=saved->fade;
    vdpVar1=saved->transfers;vdpVar2=saved->head;vdpVar3=saved->tail;
    const auto& light=saved->light;
    setupLight(light.m0,light.m4,light.m8,light.mC);
    generateLightFalloffMap(light.m10,light.m14,light.m18);
    dragonFieldTaskDrawSub1Sub1(light.m1C_pointLightX,light.m20_pointLightY,
        light.m24_pointLightZ,light.m28_pointLightParam);
    std::memcpy(getVdp2Cram(0),saved->cram,sizeof(saved->cram));
    std::memcpy(getVdp2Vram(0x3E000),saved->vram,sizeof(saved->vram));
    if(saved->includeFade){
        std::memcpy(getVdp2Vram(0x3C000),saved->optionalVram,0x400);
        std::memcpy(getVdp2Vram(0x3E000),saved->optionalVram+0x400,0x400);
    }
    setVdp2VramU16((vdp2Controls.m4_pendingVdp2Regs->mAC_BKTA&0x7FFFF)*2,saved->backColor);
    setVdp2VramU16((vdp2Controls.m4_pendingVdp2Regs->mA8_LCTA&0x7FFFF)*2,saved->lineColor);
    lagiDiscardBattleGraphics(backup);
    return true;
}

void lagiDiscardBattleGraphics(void*& backup)
{
    delete static_cast<BattleGraphics*>(backup);
    backup=nullptr;
}
