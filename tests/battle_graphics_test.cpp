#include <array>
#include "lagi/battle_loading_cover.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <new>
using u8=uint8_t;using u16=uint16_t;using s16=int16_t;
// Service-boundary mocks, not ABI/layout verification. Vita compilation uses
// real native types; this test executes the unchanged production function body.
struct s_vdp1Command {s16 mC_CMDXA,mE_CMDYA,m14_CMDXC,m16_CMDYC;};
struct s_graphicEngineStatus_405C {int value; s_vdp1Command* setClippingCoordinatesEA; s_vdp1Command* setLocalCoordinatesEA;};
struct {s_graphicEngineStatus_405C m405C;} graphicEngineStatus{};
struct Regs {unsigned mAC_BKTA,mA8_LCTA;int extra[300];};
struct sVdp2Controls {unsigned m0_doubleBufferIndex;Regs* m4_pendingVdp2Regs;int extra[30];Regs m20_registers[2];} vdp2Controls{};
struct sFadeControls {int value; } g_fadeControls{};
struct sVdpVar1 {void* m0_source[2];u8* m8_destination;int mC_size;bool mE_isDoubleBuffered,mF_isPending;sVdpVar1* m10_nextTransfert;};
std::array<sVdpVar1,14> vdpVar1{};sVdpVar1* vdpVar2=nullptr; sVdpVar1* vdpVar3=nullptr;
struct s_lightSetup {int m0,m4,m8,mC;unsigned m10,m14,m18;int m1C_pointLightX,m20_pointLightY,m24_pointLightZ;unsigned m28_pointLightParam;} lightSetup{};
std::array<u8,0x100000> vram{};std::array<u8,0x1000> cram{};
u8* getVdp2Vram(unsigned at){assert(at<vram.size());return vram.data()+at;}
u8* getVdp2Cram(unsigned at){return cram.data()+at;}
u16 getVdp2VramU16(unsigned at){return vram[at]*256+vram[at+1];}
void setVdp2VramU16(unsigned at,u16 value){vram[at]=value>>8;vram[at+1]=value;}
int lightCall=0;
void setupLight(int a,int b,int c,unsigned d){assert(lightCall++==0);lightSetup.m0=a;lightSetup.m4=b;lightSetup.m8=c;lightSetup.mC=d;}
void generateLightFalloffMap(unsigned a,unsigned b,unsigned c){assert(lightCall++==1);lightSetup.m10=a;lightSetup.m14=b;lightSetup.m18=c;}
void dragonFieldTaskDrawSub1Sub1(int a,int b,int c,unsigned d){assert(lightCall++==2);lightSetup.m1C_pointLightX=a;lightSetup.m20_pointLightY=b;lightSetup.m24_pointLightZ=c;lightSetup.m28_pointLightParam=d;}
void lagiDiscardBattleGraphics(void*&);
namespace lagi::resident_video {
struct Snapshot {};
bool resourceReady=true; int captures=0,restores=0;
void capture(Snapshot&){++captures;}
bool prepare(Snapshot&){return resourceReady;}
bool restore(Snapshot&){if(!resourceReady)return false;++restores;return true;}
}
#include "service.inc"
int main(){
    for(int optional=0;optional<2;++optional)for(int pending=0;pending<2;++pending){
        vdp2Controls={};vdpVar1={};lightCall=0;
        vdp2Controls.m0_doubleBufferIndex=1-pending; // pending owner is independent
        vdp2Controls.m4_pendingVdp2Regs=&vdp2Controls.m20_registers[pending];
        auto& regs=*vdp2Controls.m4_pendingVdp2Regs;
        regs.mAC_BKTA=0x80000120;regs.mA8_LCTA=0x40000140;
        vdp2Controls.m20_registers[0].extra[299]=1234;
        vdp2Controls.m20_registers[1].extra[299]=-5678;
        vdpVar2=&vdpVar1[0];vdpVar3=&vdpVar1[13];
        vdpVar1[0].m10_nextTransfert=&vdpVar1[13];
        vdpVar1[13].mF_isPending=true;vdpVar1[13].mC_size=17;
        vdpVar1[13].m0_source[0]=cram.data();
        s_vdp1Command oldLocal{1,2,0,0},oldClip{3,4,5,6};
        graphicEngineStatus.m405C={42,&oldClip,&oldLocal};
        lightSetup={1,2,3,4,5,6,7,8,9,10,11};
        g_fadeControls.value=25;
        cram.fill(0xAC);vram.fill(0xBD);
        setVdp2VramU16(0x240,0x1234);setVdp2VramU16(0x280,0x5678);
        void* saved=lagiCaptureBattleGraphics(optional!=0);assert(saved);
        assert(lagi::battleLoadingCover.active && !lagi::battleLoadingCover.loading);
        lagi::resident_video::resourceReady=false;
        assert(!lagiPrepareBattleGraphicsRestore(saved));
        assert(!lagiRestoreBattleGraphics(saved) && saved);
        assert(lightCall==0); // no graphics mutation after failed resource stage
        lagi::resident_video::resourceReady=true;
        assert(lagiPrepareBattleGraphicsRestore(saved));
        s_vdp1Command newLocal{},newClip{};
        graphicEngineStatus.m405C={99,&newClip,&newLocal};
        oldLocal={};oldClip={}; // old list has been reused
        vdp2Controls={};vdpVar1={};vdpVar2=nullptr;vdpVar3=nullptr;
        lightSetup={};g_fadeControls.value=77;cram.fill(0);vram.fill(0);
        assert(lagiRestoreBattleGraphics(saved));assert(!saved);
        assert(!lagi::battleLoadingCover.active);
        assert(graphicEngineStatus.m405C.value==42);
        assert(graphicEngineStatus.m405C.setLocalCoordinatesEA==&newLocal);
        assert(graphicEngineStatus.m405C.setClippingCoordinatesEA==&newClip);
        assert(newLocal.mC_CMDXA==1 && newLocal.mE_CMDYA==2);
        assert(newClip.mC_CMDXA==3 && newClip.mE_CMDYA==4 && newClip.m14_CMDXC==5 && newClip.m16_CMDYC==6);
        assert(oldLocal.mC_CMDXA==0 && oldClip.mC_CMDXA==0);
        assert(vdp2Controls.m4_pendingVdp2Regs==&vdp2Controls.m20_registers[pending]);
        assert(vdp2Controls.m0_doubleBufferIndex==unsigned(1-pending));
        assert(vdp2Controls.m20_registers[0].extra[299]==1234 && vdp2Controls.m20_registers[1].extra[299]==-5678);
        assert(vdpVar2==&vdpVar1[0] && vdpVar3==&vdpVar1[13]);
        assert(vdpVar1[0].m10_nextTransfert==&vdpVar1[13]);
        assert(vdpVar1[13].mF_isPending && vdpVar1[13].mC_size==17 && vdpVar1[13].m0_source[0]==cram.data());
        assert(lightCall==3 && lightSetup.m0==1 && lightSetup.m14==6 && lightSetup.m28_pointLightParam==11);
        assert(cram[0]==0xAC && cram.back()==0xAC);
        assert(vram[0x3E000]==0xBD && vram[0x3E3FF]==0xBD && vram[0x3E400]==0);
        assert(vram[0x3C000]==(optional?0xBD:0) && vram[0x3C3FF]==(optional?0xBD:0));
        assert(g_fadeControls.value==(optional?25:77));
        assert(getVdp2VramU16(0x240)==0x1234 && getVdp2VramU16(0x280)==0x5678);
        lagiRestoreBattleGraphics(saved);assert(lightCall==3);
        saved=lagiCaptureBattleGraphics(false);assert(saved);lagiDiscardBattleGraphics(saved);assert(!saved);
    }
    vdp2Controls.m4_pendingVdp2Regs=nullptr;assert(!lagiCaptureBattleGraphics(false));
    vdp2Controls.m4_pendingVdp2Regs=&vdp2Controls.m20_registers[0];
    sVdpVar1 foreign{};vdpVar2=&foreign;assert(!lagiCaptureBattleGraphics(false));
    vdpVar2=nullptr;vdpVar1[7].m10_nextTransfert=&foreign;assert(!lagiCaptureBattleGraphics(false));
    puts("PASS: typed graphics capture/restore, both pending owners, optional spans, live command pointers, queue and light callbacks; mocked services only");
}
