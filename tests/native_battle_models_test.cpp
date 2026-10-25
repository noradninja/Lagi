#include <array>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <vector>
#include "lagi/native_battle_models.h"
struct Task {bool finished=false;void markFinished(){finished=true;}};
struct s_workAreaCopy {std::array<uint8_t,192> header;s_fileBundle* m0_fileBundle=nullptr;Task task;Task* getTask(){return &task;}};
struct sSaturnPtr {uint32_t ea=0;bool isNull()const{return ea==0;}sSaturnPtr operator+(unsigned n)const{return {ea+n};}};
struct sVec3_FP {
 int32_t x=0,y=0,z=0;
 struct Ref {int32_t& m_value;int32_t asS32()const{return m_value;}};
 struct ConstRef {const int32_t& m_value;int32_t asS32()const{return m_value;}};
 Ref operator[](unsigned i){assert(i<3);return {i==0?x:i==1?y:z};}
 ConstRef operator[](unsigned i)const{assert(i<3);return {i==0?x:i==1?y:z};}
};
struct sBattleTargetable {s_workAreaCopy* owner; sVec3_FP* position;int32_t radius;uint32_t m50_flags,index,a,b;int16_t m58=0;int8_t m60=0,m5E_impactForce=0;};
struct s_hotpoinEntry {int32_t m0;sVec3_FP m4;int32_t m10;};
struct s_hotpointDefinition {std::vector<s_hotpoinEntry> m0;uint32_t m4_count=0;};
struct sHotpointBundle {sSaturnPtr address;explicit sHotpointBundle(sSaturnPtr p):address(p){}};
struct sAnimationData {uint16_t m4_numFrames=0;};
struct s_3dModel {
 void (*m18_drawFunction)(s_3dModel*)=nullptr;
 sAnimationData* m30_pCurrentAnimation=nullptr;
 uint16_t m12_numBones=0,m50_targetBoneCount=0,m52_targetableCount=0;
 std::vector<s_hotpointDefinition>* m40=nullptr;
 std::vector<std::vector<sVec3_FP>> m44_hotpointData;
 sBattleTargetable* m54_targetables=nullptr;sVec3_FP* m58_targetablePositions=nullptr;
 std::vector<sHotpointBundle> m_hotpointBundles;
};
struct Hierarchy {unsigned countNumberOfBones(){return 2;}};
struct Pose{};
struct s_fileBundle {Hierarchy hierarchy;Pose pose;sAnimationData* animation=nullptr;Hierarchy* getModelHierarchy(unsigned){return &hierarchy;}Pose* getStaticPose(unsigned,unsigned count){assert(count==2);return &pose;}sAnimationData* getAnimation(unsigned offset){assert(offset==0x9c);return animation;}};
static std::array<uint8_t,64> disc{};
static uint16_t readSaturnU16(sSaturnPtr p){assert(p.ea+1<disc.size());return uint16_t(disc[p.ea])*256+disc[p.ea+1];}
static sSaturnPtr readSaturnEA(sSaturnPtr p){assert(p.ea+3<disc.size());return {uint32_t(disc[p.ea])*0x1000000u+uint32_t(disc[p.ea+1])*0x10000u+uint32_t(disc[p.ea+2])*256u+disc[p.ea+3]};}
static int creates=0,steps=0,failModel=-1;
static bool repeatedModelDescriptor=false;
static std::vector<uint32_t> hotspotCalls;
static bool init3DModelRawData(s_workAreaCopy*,s_3dModel* m,int zero,s_fileBundle*,uint16_t model,void* animation,Pose* pose,void* color,sHotpointBundle* hotspots){
 assert(zero==0&&!animation&&pose&&!color);assert(model==0x10+(repeatedModelDescriptor?0:creates));
 hotspotCalls.push_back(hotspots?hotspots->address.ea:0);m->m12_numBones=2;
 return creates++!=failModel;
}
static unsigned stepAnimation(s_3dModel*){++steps;return 0;}
static std::vector<void*> allocations;
static unsigned targetCalls=0,retired=0;
static void* allocateHeapForTask(s_workAreaCopy*,unsigned bytes){if(!bytes)return nullptr;auto p=std::calloc(1,bytes);assert(p);allocations.push_back(p);return p;}
static void initTargetable(sBattleTargetable* t,s_workAreaCopy* owner,sVec3_FP* pos,int32_t radius,uint32_t flags,uint32_t index,uint32_t a,uint32_t b){
 assert(pos->x==0&&pos->y==0&&pos->z==0);*t={owner,pos,radius,flags,index,a,b};++targetCalls;
}
static void deleteTargetable(sBattleTargetable* t){assert(t->owner);t->m50_flags|=0x40000;++retired;}
#include "../src/integration/lagi_battle_models.inc"
static void release(){for(auto p:allocations)std::free(p);allocations.clear();targetCalls=retired=0;}
static void descriptor(unsigned entry,unsigned model,uint32_t hotspots){
 auto i=entry*8;disc[i]=model>>8;disc[i+1]=model;disc[i+2]=0;disc[i+3]=0x20+entry;
 for(unsigned b=0;b<4;++b)disc[i+4+b]=hotspots>>(24-8*b);
}
static uint32_t* callbackFlags=nullptr;static unsigned callbacks=0;static bool clearGate=false;
static void callback(s_workAreaCopy*){++callbacks;assert((*callbackFlags&0xc0000000u)==0x80000000u);if(clearGate)*callbackFlags&=~0x10000000u;}
int main(){
 s_workAreaCopy owner;owner.header.fill(0xa5);
 s_fileBundle bundle;
 for(int failure=-1;failure<3;++failure){
  for(unsigned i=0;i<3;++i)descriptor(i,0x10+i,i==1?0:0x6000000u+i*16);
  creates=steps=0;hotspotCalls.clear();failModel=failure;owner.task.finished=false;
  uint8_t count=99;std::vector<s_3dModel> models;
  assert(lagiCreateBattleEnemyModels(&owner,&bundle,count,models,{0},3)==(failure<0));
  assert(count==3&&models.size()==3&&steps==(failure<0?3:failure));
  assert(creates==(failure<0?3:failure+1));assert(owner.task.finished==(failure>=0));
  for(int i=0;i<creates;++i){assert(hotspotCalls[i]==(i==1?0:0x6000000u+i*16));assert(models[i].m_hotpointBundles.size()==(i==1?0:1));}
 }
 size_t cases=0;
 for(unsigned entity=0;entity<128;++entity)for(unsigned variation=0;variation<64;++variation){
  owner.task.finished=false;
  std::vector<s_hotpointDefinition> data(3);
  data[0].m4_count=2;data[0].m0={{int32_t(variation*0x04000001u),{999,999,999},-123},{int32_t(~variation),{-555,-555,-555},0x7000}};
  data[1].m4_count=0;
  data[2].m4_count=1;data[2].m0={{int32_t(entity*0x01010101u),{0,0,0},-1}};
  std::vector<s_3dModel> models(2);auto& m=models[0];m.m12_numBones=3;m.m40=&data;m.m44_hotpointData={{{1,2,3},{4,5,6}},{},{{7,8,9}}};
  models[1].m12_numBones=7;
  LagiBattleTargetOverride oldOverride{};
  LagiBattleModelTargetState state;state.healthFloor=99;state.hitboxValues.fill(-99);state.auxiliaryCount=7;state.auxiliaryData=&oldOverride;
  const std::array<uint32_t,4> ea={0x0609f440,0x0609f444,0x0609f468,0x0609f48c};
  assert(lagiInitBattleEnemyModelTargets(&owner,entity,models,state,ea));
  assert(!owner.task.finished&&state.healthFloor==0&&state.damageCallbackEA==ea&&state.auxiliaryCount==0&&!state.auxiliaryData);
  for(auto v:state.hitboxValues)assert(v==10);
  assert(targetCalls==3&&m.m50_targetBoneCount==3&&m.m52_targetableCount==3);
  assert(models[1].m50_targetBoneCount==7&&!models[1].m54_targetables&&!models[1].m58_targetablePositions);
  unsigned j=0;for(auto& bone:data)for(auto& point:bone.m0){auto& t=m.m54_targetables[j];assert(t.owner==&owner&&t.position==m.m58_targetablePositions+j&&t.index==entity&&t.radius==point.m10&&t.m50_flags==uint32_t(point.m0)&&t.a==0&&t.b==10);++j;}
  uint32_t flags=0x40808005u;callbackFlags=&flags;callbacks=0;clearGate=false;
  lagiRetireBattleEnemyModelTargets(&owner,flags,models,callback);assert(retired==3&&callbacks==1&&flags==0x90808005u);
  auto* storage=m.m54_targetables;lagiRetireBattleEnemyModelTargets(&owner,flags,models,callback);assert(retired==3&&callbacks==2&&m.m54_targetables==storage);
  clearGate=true;lagiRetireBattleEnemyModelTargets(&owner,flags,models,callback);assert(retired==6&&callbacks==3);clearGate=false;
  for(auto byte:owner.header)assert(byte==0xa5);release();++cases;
 }
 // A later malformed model must not leave targets from an earlier valid one.
 for(int fault=0;fault<4;++fault){
  std::vector<s_hotpointDefinition> good(1),bad(1);good[0].m4_count=1;good[0].m0.resize(1);bad=good;
  std::vector<s_3dModel> models(2);for(auto& m:models){m.m12_numBones=1;m.m44_hotpointData={{{}}};}models[0].m40=&good;models[1].m40=&bad;
  if(fault==0)bad.clear();if(fault==1)bad[0].m0.clear();if(fault==2)models[1].m44_hotpointData.clear();if(fault==3)bad[0].m4_count=0x8000;
  LagiBattleModelTargetState state;owner.task.finished=false;
  assert(!lagiInitBattleEnemyModelTargets(&owner,1,models,state,{}));assert(owner.task.finished&&targetCalls==0&&allocations.empty());
 }
 std::printf("PASS: native model creation/failure and %zu metadata/identity/retirement cases; mocked engine and task services, not hardware\n",cases);
 return 0;
}
