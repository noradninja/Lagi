#include "lagi/lagi_menu_nbg2.h"
#include <cassert>
#include <cstdio>
static unsigned g_vdp2MenuId=0,g_profileRenderUs=25000,scenePerfHeartbeat=0;
static bool g_vdp2SaveLayout=false;
static lagi::MenuNbg2State g_menuNbg2;
static bool sample(bool nativeSceneMode,bool nativeMenuFrame) {
#include "gate.inc"
 return false;
}
int main() {
 assert(!sample(false,false));
 g_vdp2MenuId=1;
 assert(sample(false,true)); // UI-only entry, no world geometry.
 for(unsigned i=0;i<59;++i)assert(!sample(false,true));
 assert(sample(false,true)); // UI-only heartbeat continues advancing.
 g_menuNbg2.bgon=4;g_menuNbg2.prinb=5;g_menuNbg2.chctlb=1;g_menuNbg2.pncn=0x800C;
 assert(sample(false,true)); // Map transition between heartbeats.
 g_profileRenderUs=34000;
 for(unsigned i=0;i<4;++i)assert(sample(false,true));
 for(unsigned i=0;i<8;++i)assert(!sample(false,true)); // Extra samples bounded.
 g_vdp2SaveLayout=true;
 assert(sample(false,true)); // Save transition replenishes the budget.
 for(unsigned i=0;i<3;++i)assert(sample(false,true));
 assert(!sample(false,true));
 g_vdp2MenuId=0;g_vdp2SaveLayout=false;g_menuNbg2={};
 assert(sample(true,false)); // Return to world presentation.
 const auto heartbeat=scenePerfHeartbeat;
 for(unsigned i=0;i<120;++i)assert(!sample(false,false));
 assert(scenePerfHeartbeat==heartbeat);
 std::puts("PASS: production profile gate captures UI-only Map/menus, heartbeat, state transitions and bounded overruns without profiling invalid frames");
}
