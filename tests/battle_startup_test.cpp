#include "lagi/battle_startup.h"
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
using u8=std::uint8_t;
struct s_workArea{};
struct Manager{int m2_currentBattleOverlayId=0,m6_subBattleId=0;}manager;
Manager* gBattleManager=nullptr;
struct Entry{std::string m4_prg;unsigned mC_numSubBattles;std::string m8_fnt="A3.FNT";};
struct Common{std::vector<Entry> battleOverlaySetup;}common;
Common* gCommonFile=nullptr;
int selectedDisc=1,initializations=0,entries=0,reports=0;
bool loadSucceeds=true,allocationSucceeds=true;
unsigned resourceBytes=lagi::battle_generic_minimum_bytes;
unsigned getFileSize(const char* name){assert(std::string(name)=="BTL_A3.PRG");return resourceBytes;}
struct BTL_GenericData{u8* m_data=nullptr;unsigned m_dataSize=0;static void staticInit();};
BTL_GenericData* g_BTL_GenericData=nullptr;
void BTL_GenericData::staticInit(){
    assert(!g_BTL_GenericData);++initializations;g_BTL_GenericData=new BTL_GenericData;
    if(loadSucceeds){g_BTL_GenericData->m_data=new u8[1];g_BTL_GenericData->m_dataSize=resourceBytes;}
}
namespace lagi::disc {int selected_disc(){return selectedDisc;}}
namespace lagi::platform::logging {template<class... T>void writef(const char*,T...){++reports;}}
s_workArea* expectedParent=nullptr;
s_workArea* overlayStart_BTL_A3(s_workArea* parent){assert(parent==expectedParent);++entries;return allocationSucceeds?parent:nullptr;}
#include "service.inc"
struct sBattleOverlayTask:s_workArea {int m0=1,m2_numLoadedFnt=0;};
struct {int m4=0;}graphicEngineStatus;
std::string sBattleOverlayName;int fontLoads=0;
void loadFnt(const char*){assert(g_BTL_GenericData);++fontLoads;}
#include "overlay.inc"
int main(){
    assert(!lagiPrepareNativeA3Battle() && initializations==0);
    const int firstError=reports;assert(!lagiPrepareNativeA3Battle() && reports==firstError);
    gBattleManager=&manager;gCommonFile=&common;
    assert(!lagiPrepareNativeA3Battle());
    common.battleOverlaySetup={{"BTL_A3.PRG",10},{"BTL_A3_2.PRG",2}};
    manager.m2_currentBattleOverlayId=-1;assert(!lagiPrepareNativeA3Battle());
    manager.m2_currentBattleOverlayId=1;assert(!lagiPrepareNativeA3Battle() && initializations==0);
    manager.m2_currentBattleOverlayId=0;manager.m6_subBattleId=10;assert(!lagiPrepareNativeA3Battle());
    manager.m6_subBattleId=-1;assert(!lagiPrepareNativeA3Battle());
    manager.m6_subBattleId=3;resourceBytes=0;assert(!lagiPrepareNativeA3Battle() && initializations==0);
    resourceBytes=lagi::battle_generic_minimum_bytes;loadSucceeds=false;
    assert(!lagiPrepareNativeA3Battle() && initializations==1 && !g_BTL_GenericData);
    loadSucceeds=true;assert(lagiPrepareNativeA3Battle() && initializations==2 && g_BTL_GenericData);
    assert(lagiPrepareNativeA3Battle() && initializations==2);
    s_workArea parent;expectedParent=&parent;allocationSucceeds=false;
    assert(!lagiEnterNativeA3Battle(&parent) && entries==1);
    const auto allocationError=reports;
    assert(lagiPrepareNativeA3Battle());assert(!lagiEnterNativeA3Battle(&parent) && reports==allocationError);
    allocationSucceeds=true;assert(lagiEnterNativeA3Battle(&parent) && entries==3);
    ++selectedDisc;assert(!lagiPrepareNativeA3Battle() && initializations==2);
    --selectedDisc;
    sBattleOverlayTask task;expectedParent=&task;allocationSucceeds=false;
    battleOverlayTask_Draw(&task);assert(task.m0==1 && fontLoads==2);
    battleOverlayTask_Draw(&task);assert(task.m0==1 && fontLoads==2);
    allocationSucceeds=true;battleOverlayTask_Draw(&task);
    assert(task.m0==2 && fontLoads==2);const int started=entries;
    battleOverlayTask_Draw(&task);assert(entries==started && task.m0==2);
    delete[] g_BTL_GenericData->m_data;delete g_BTL_GenericData;g_BTL_GenericData=nullptr;
    puts("PASS: production A3 preflight and generated native overlay Draw: generic-data retry/once-init, authored IDs, unsupported overlay/disc rejection, factory retry without duplicate fonts, running-state handoff; mocked engine/data parser");
}
