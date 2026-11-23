from pathlib import Path
import subprocess,tempfile
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_single.cpp').read_text()
a=s.index('p_workArea lagiCreateGarilSingleActor(');b=s.index('void battleEngine_PlayAttackCamera(int);',a)
factory=s[a:b];assert 'pParent->' not in factory
fixture=r'''
#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include "lagi/garil_actor_group.h"
#include "lagi_battle_enemy_lifecycle.h"
using u8=uint8_t;using u32=uint32_t;using s32=int32_t;using s16=int16_t;
struct s_workArea{};struct s_workAreaCopy:s_workArea{};using p_workArea=s_workArea*;
struct sVec3_FP{int m0_X=0,m4_Y=0,m8_Z=0;};
struct sSaturnPtr{unsigned address=0;};
struct sEntityGroup{int mC_formationNameIndex=0;sSaturnPtr m8_dataTable;};
struct sLagiGarilSingleFormation:s_workAreaCopy{sEntityGroup mE0_entityGroup;u8 mF0_flag=0;std::array<u8,3> mF1_pad5{};u8 mF4_entityPositions[24]{};std::array<std::array<sVec3_FP,6>,3> m_lagiLayouts{};unsigned m_lagiLayout=0;};
struct s_fileBundle{}bundle;struct Resource{s_fileBundle* m0_fileBundle=&bundle;}resource;struct Slot{Resource* mC_fileBundle=&resource;};Slot dramAllocatorEnd[13];
struct Meter{}meter;
struct sBTL_A5_SingleEnemy:s_workAreaCopy{
 struct TypedTaskDefinition{void(*init)(sBTL_A5_SingleEnemy*);void(*update)(sBTL_A5_SingleEnemy*);void(*draw)(sBTL_A5_SingleEnemy*);void(*destroy)(sBTL_A5_SingleEnemy*);};
 s_fileBundle* m0_fileBundle=nullptr;LagiGarilSingleGroupBinding m_lagiGroup;int m1C_entityIndex=0,m1BC_isLead2=0,m17C_hitFlashCount=0,m17E_hitFlashColor=0,mA8_soundEffectId=0;s16 mB2_health=0;Meter* mAC_lifeMeter=nullptr;sVec3_FP m3C_rotation,m84_displayRotation,m9C_targetRotation,m90_initialPosition,m60_targetPosition,m30_position;
};
void BTL_A5_singleEnemy_Update(sBTL_A5_SingleEnemy*){}void BTL_A5_singleEnemy_Draw(sBTL_A5_SingleEnemy*){}void BTL_A5_singleEnemy_Delete(sBTL_A5_SingleEnemy*){}
s_workAreaCopy* seenParent=nullptr;bool failCreate=false,failModels=false;int seenId=-1;
template<class T>T* createSubTaskWithCopy(s_workAreaCopy* p,const typename T::TypedTaskDefinition* d){seenParent=p;assert(d->update==&BTL_A5_singleEnemy_Update&&d->draw==&BTL_A5_singleEnemy_Draw&&d->destroy==&BTL_A5_singleEnemy_Delete);return failCreate?nullptr:new T;}
Meter* createEnemyLifeMeterTask(sVec3_FP*,int zero,s16* health,int id){assert(!zero);*health=123;seenId=id;return &meter;}
bool lagiInitGarilSingleModels(sBTL_A5_SingleEnemy*,s_fileBundle* p,int lead){assert(p==&bundle&&(lead==0||lead==1));return !failModels;}
struct Engine{sVec3_FP mC_battleCenter={111,222,333};}engine;using s_battleEngine=Engine;
struct Overlay{Engine* m4_battleEngine=&engine;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
// FACTORY
int main(){
 s_workAreaCopy parent;std::array<sEntityGroup,2> groups;groups[0].mC_formationNameIndex=15;groups[1].mC_formationNameIndex=17;std::array<sVec3_FP,2> positions={sVec3_FP{10,20,30},sVec3_FP{100,200,300}};std::array<u8,2> active{},reaction{};std::array<std::array<u8,24>,2> dead{};
 for(unsigned group=0;group<2;++group)for(int lead=0;lead<2;++lead){LagiGarilSingleGroupBinding binding{&groups[group],&active[group],&reaction[group],dead[group].data(),&positions[group],[](void* p,int index)->sVec3_FP{assert(index==2);return *static_cast<sVec3_FP*>(p);}};
  auto* a=static_cast<sBTL_A5_SingleEnemy*>(lagiCreateGarilSingleActor(&parent,binding,2,lead));assert(a&&seenParent==&parent&&seenId==groups[group].mC_formationNameIndex+lead);assert(a->m_lagiGroup.group==&groups[group]&&a->m_lagiGroup.activeMask==&active[group]&&a->mB2_health==123&&a->mAC_lifeMeter==&meter);assert(a->m90_initialPosition.m0_X==positions[group].m0_X&&a->m30_position.m0_X==positions[group].m0_X+111&&a->m30_position.m4_Y==positions[group].m4_Y+222&&a->m30_position.m8_Z==positions[group].m8_Z+333);assert(a->mA8_soundEffectId==0x51E&&a->m1BC_isLead2==lead);delete a;
 }
 sLagiGarilSingleFormation baseline;baseline.mE0_entityGroup.mC_formationNameIndex=20;baseline.m_lagiLayout=2;baseline.m_lagiLayouts[2][1]={1234,5678,9012};auto* a=static_cast<sBTL_A5_SingleEnemy*>(BTL_A5_createEnemySingle(&baseline,1,1));assert(a&&a->m_lagiGroup.group==&baseline.mE0_entityGroup&&seenId==21&&a->m90_initialPosition.m0_X==1234);baseline.m_lagiLayout=0;baseline.m_lagiLayouts[0][1]={777,0,0};assert(a->m_lagiGroup.targetPosition(a->m_lagiGroup.positionContext,1).m0_X==777);delete a;
 failCreate=true;assert(!BTL_A5_createEnemySingle(&baseline,1,0));
 puts("PASS production shared single factory: unrelated parent task, independent group IDs/bindings, both actor kinds, existing wrapper and live layout changes");
}
'''.replace('// FACTORY',factory)
with tempfile.TemporaryDirectory(prefix='lagi-single-factory-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
