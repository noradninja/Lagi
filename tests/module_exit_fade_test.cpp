#include <cassert>
#include <cstdio>
#include <initializer_list>
void PDS_Log(const char*,...) {}
struct Channel {int m0_color=42,m20_stopped=1;};
struct {Channel m0_fade0,m24_fade1;int m_4D=5,m_4C=0;} g_fadeControls;
struct {int m8_nextGameStatus=0,m0_gameMode=3,m2=0,m4_gameStatus=0;} gGameStatus;
struct s_moduleManager {void* m8=reinterpret_cast<void*>(1);};
struct Common {int getSaturnPtr(int n){return n;}} common,*gCommonFile=&common;
static int destination=1,calls=0,expectedSteps=30;
int readSaturnS8(int at){assert(at==0x212EAC+gGameStatus.m8_nextGameStatus*2);return destination;}
int convertColorToU32ForFade(int color){return color;}
int fadePalette(Channel* channel,int from,int to,int steps){
 assert(from==42 && to==0 && steps==expectedSteps);++calls;channel->m20_stopped=0;return 1;
}
#include "../src/integration/lagi_module_exit_fade.inc"
int main(){
 s_moduleManager manager;
 for(int mode:{1,2,3,4}) for(int target:{0,1,2}) {
  gGameStatus={0,mode,0};assert(!lagiWaitForModuleExitFade(&manager));
  destination=target;calls=0;gGameStatus.m8_nextGameStatus=8;
  assert(lagiWaitForModuleExitFade(&manager));assert(calls==2);
  for(int i=0;i<30;++i)assert(lagiWaitForModuleExitFade(&manager));
  assert(calls==2);g_fadeControls.m0_fade0.m20_stopped=1;
  assert(lagiWaitForModuleExitFade(&manager));
  g_fadeControls.m24_fade1.m20_stopped=1;
  assert(!lagiWaitForModuleExitFade(&manager));assert(calls==2);
 }
 gGameStatus={0,3,0};lagiWaitForModuleExitFade(&manager);calls=0;
 // Reference-confirmed camera exit: no additional 30-update interpolation.
 destination=1;expectedSteps=1;gGameStatus={6,3,0,0x50};
 assert(lagiWaitForModuleExitFade(&manager)&&calls==2);
 assert(lagiWaitForModuleExitFade(&manager)&&calls==2);
 g_fadeControls.m0_fade0.m20_stopped=1;
 assert(lagiWaitForModuleExitFade(&manager));
 g_fadeControls.m24_fade1.m20_stopped=1;
 assert(!lagiWaitForModuleExitFade(&manager));
 gGameStatus={0,3,0};lagiWaitForModuleExitFade(&manager);calls=0;expectedSteps=30;
 gGameStatus.m8_nextGameStatus=8;g_fadeControls.m_4C=6;
 assert(lagiWaitForModuleExitFade(&manager)&&calls==0);
 g_fadeControls.m_4C=0;assert(lagiWaitForModuleExitFade(&manager)&&calls==2);
 // Cancel/retry, battle/field, save frontend and pushed-overlay requests.
 gGameStatus.m8_nextGameStatus=0;assert(!lagiWaitForModuleExitFade(&manager));
 calls=0;gGameStatus.m8_nextGameStatus=9;
 for(int target:{3,4,5,9}){destination=target;assert(!lagiWaitForModuleExitFade(&manager));}
 destination=1;gGameStatus.m0_gameMode=9;assert(!lagiWaitForModuleExitFade(&manager));
 gGameStatus.m0_gameMode=3;gGameStatus.m2=1;assert(!lagiWaitForModuleExitFade(&manager));
 gGameStatus.m2=0;manager.m8=nullptr;assert(!lagiWaitForModuleExitFade(&manager));assert(calls==0);
 manager.m8=reinterpret_cast<void*>(1);
 g_fadeControls.m0_fade0={0x8000,1};g_fadeControls.m24_fade1={0x8000,1};
 assert(!lagiWaitForModuleExitFade(&manager)&&calls==0); // no duplicate blackout hold
 puts("PASS production exit-fade gate: native endpoints/count, both completion flags, no restart, priority, cancellation and excluded transitions; no hardware acceptance");
}
