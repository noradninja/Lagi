#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <map>
#include <vector>
using s8=int8_t;using u8=uint8_t;using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
struct fixedPoint{int32_t value=0;fixedPoint(int32_t v=0):value(v){}int32_t asS32()const{return value;}};
struct sVec3_FP{fixedPoint x,y,z;};
struct File;
struct sSaturnPtr{u32 m_offset=0;File* m_file=nullptr;sSaturnPtr operator+(unsigned v)const{return {m_offset+v,m_file};}bool isNull()const{return !m_offset;}};
struct File{std::vector<u8> bytes;sSaturnPtr getSaturnPtr(u32 v){return {v,this};}} original;
File* g_BTL_A5_3=&original;
u16 readSaturnU16(sSaturnPtr p){const auto i=p.m_offset-0x6054000;return (p.m_file->bytes.at(i)<<8)|p.m_file->bytes.at(i+1);}
u8 readSaturnU8(sSaturnPtr p){return p.m_file->bytes.at(p.m_offset-0x6054000);}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {(u32(readSaturnU16(p))<<16)|readSaturnU16(p+2),p.m_file};}
sVec3_FP readSaturnVec3(sSaturnPtr p){return {s32(readSaturnEA(p).m_offset),s32(readSaturnEA(p+4).m_offset),s32(readSaturnEA(p+8).m_offset)};}
struct s_workAreaCopy{struct Task{bool finished=false;void markFinished(){finished=true;}}task;Task* getTask(){return &task;}struct s_fileBundle* m0_fileBundle=nullptr;void* m4_vd1Allocation=nullptr;};
using p_workArea=s_workAreaCopy*;
struct s_hotpoinEntry{int m0=0,m10=0;};
struct s_hotpointDefinition{unsigned m4_count=0;std::vector<s_hotpoinEntry> m0;};
struct sHotpointBundle{
 sSaturnPtr ea;std::vector<s_hotpointDefinition> definitions;
 sHotpointBundle(sSaturnPtr p):ea(p){}
 std::vector<s_hotpointDefinition>* getData(unsigned count){
  definitions.clear();for(unsigned bone=0;bone<count;++bone){auto ptr=readSaturnEA(ea+bone*8);unsigned n=readSaturnEA(ea+bone*8+4).m_offset;s_hotpointDefinition d;d.m4_count=n;assert(n<10);
   for(unsigned i=0;i<n;++i)d.m0.push_back({int(readSaturnEA(ptr+i*20).m_offset),int(readSaturnEA(ptr+i*20+16).m_offset)});
   definitions.push_back(d);}return &definitions;
 }
};
struct sBattleTargetable{s_workAreaCopy* m0=nullptr;sVec3_FP* m4_pPosition=nullptr;u32 flags=0;int radius=0;};
struct sAnimationData{u16 flags,bones,frames;u16 key;};
struct sStaticPoseData{u16 key;unsigned bones;};
struct sModelHierarchy{unsigned bones;unsigned countNumberOfBones(){return bones;}};
struct s_fileBundle{
 std::vector<u8> bytes;std::map<u16,sModelHierarchy> hierarchies;std::map<u16,sStaticPoseData> poses;std::map<u16,sAnimationData> animations;
 u32 word(unsigned i){assert(i+4<=bytes.size());return (u32(bytes[i])<<24)|(u32(bytes[i+1])<<16)|(bytes[i+2]<<8)|bytes[i+3];}
 u16 half(unsigned i){return (bytes.at(i)<<8)|bytes.at(i+1);}
 unsigned count(unsigned i){if(!i)return 0;return 1+count(word(i+4))+count(word(i+8));}
 sModelHierarchy* getModelHierarchy(u16 key){auto& h=hierarchies[key];h.bones=count(word(key));return &h;}
 sStaticPoseData* getStaticPose(u16 key,unsigned n){assert(word(key)+n*36<=bytes.size());poses[key]={key,n};return &poses[key];}
 sAnimationData* getAnimation(u16 key){auto off=word(key);animations[key]={half(off),half(off+2),half(off+4),key};return &animations[key];}
} bundle;
struct s_3dModel{unsigned m12_numBones=0;std::vector<sHotpointBundle> m_hotpointBundles;std::vector<s_hotpointDefinition>* m40=nullptr;std::vector<std::vector<sVec3_FP>> m44_hotpointData;sAnimationData* animation=nullptr;sStaticPoseData* pose=nullptr;unsigned key=0;};
int stepCalls=0,modelInitCalls=0;unsigned failModel=~0u;
int init3DModelRawData(s_workAreaCopy*,s_3dModel* out,int flags,s_fileBundle* b,u16 key,sAnimationData* animation,sStaticPoseData* pose,void*,sHotpointBundle* hotpoints){
 assert(!flags&&b==&bundle);++modelInitCalls;if(failModel==key)return 0;
 out->key=key;out->m12_numBones=b->getModelHierarchy(key)->bones;out->animation=animation;out->pose=pose;
 assert(out->m12_numBones==21&&pose->bones==21);
 if(hotpoints){out->m40=hotpoints->getData(out->m12_numBones);for(auto& d:*out->m40)out->m44_hotpointData.emplace_back(d.m4_count);}
 return 1;
}
void stepAnimation(s_3dModel*){++stepCalls;}
#include "model_creation.inc"
int registered=0;
void initTargetable(sBattleTargetable* t,s_workAreaCopy* owner,sVec3_FP* pos,s32 radius,u32 flags,u32 entity,u32 unused,u32 hitbox){assert(!owner&&pos&&!entity&&!unused&&hitbox==10);*t={owner,pos,flags,radius};++registered;}
#include "core_targets.inc"
struct npcFileDeleter{s_fileBundle* m0_fileBundle=&bundle;void* m4_vd1Allocation=reinterpret_cast<void*>(0x1234);} npc;
bool missingFile=false;int npcCalls=0;
npcFileDeleter* allocateNPC(s_workAreaCopy*,int slot){assert(slot==8);++npcCalls;return missingFile?nullptr:&npc;}
struct sEnemyLifeMeterTask{sVec3_FP* position;s16* health;int type;} meter;
std::vector<u8> common;int meters=0;
sEnemyLifeMeterTask* createEnemyLifeMeterTask(sVec3_FP* pos,int arg,s16* health,int type){assert(!arg&&type==22);auto i=0x179C+type*8;*health=s16((common.at(i)<<8)|common.at(i+1));meter={pos,health,type};++meters;return &meter;}
void initAnimation(s_3dModel* model,sAnimationData* anim){assert(model->key==4&&anim->key==0x150);model->animation=anim;}
u32 rngValue=0;int rngCalls=0;u32 randomNumber(){++rngCalls;return rngValue;}
s32 performModulo2(s32 divisor,s32 dividend){return dividend%divisor;}
int names=0;void displayFormationName(int a,int b,int c){assert(a==0&&b==1&&c==13);++names;}
#include "rig.inc"
std::vector<u8> load(const char* file){std::ifstream in(file,std::ios::binary);assert(in);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char**argv){
 assert(argc==4);original.bytes=load(argv[1]);bundle.bytes=load(argv[2]);common=load(argv[3]);
 for(unsigned seed=0;seed<18;++seed){
  s_workAreaCopy owner;LagiLathumRig rig;registered=stepCalls=modelInitCalls=meters=rngCalls=names=0;rngValue=seed;
  assert(lagiInitializeLathumRig(&owner,rig));assert(!owner.task.finished&&owner.m0_fileBundle==&bundle&&owner.m4_vd1Allocation==npc.m4_vd1Allocation);
  assert(rig.modelCount==1&&rig.models.size()==1&&modelInitCalls==2&&stepCalls==1&&meters==1&&names==1&&rngCalls==1);
  assert(rig.localPosition.x.value==0&&rig.localPosition.y.value==0&&rig.localPosition.z.value==-0x64000);
  assert(rig.position.x.value==0&&rig.position.y.value==0&&rig.position.z.value==0);
  assert(rig.coreTargetCount==11&&rig.coreTargets.size()==11&&rig.corePositions.size()==11);
  for(unsigned i=0;i<rig.coreTargets.size();++i)assert(rig.coreTargets[i].m4_pPosition==&rig.corePositions[i]&&rig.coreTargets[i].flags==0xF0000001u&&rig.coreTargets[i].radius==0x1000);
  assert(rig.models[0].m_hotpointBundles.size()==1&&rig.models[0].m12_numBones==21&&rig.models[0].animation->frames==149);
  assert(rig.emergenceModel.key==12&&rig.emergenceModel.pose->key==0x14C&&rig.emergenceModel.animation->key==0x158&&rig.emergenceModel.animation->frames==149);
  assert(rig.lifeMeter==&meter&&meter.position==&rig.position&&meter.health==&rig.health&&rig.health>0);
  assert(rig.selectedGroup==seed%9&&rig.groupCounter==-1&&rig.mode==4&&rig.modelPhase==0);
  const std::array<unsigned,9> counts{1,2,1,2,1,2,1,1,2};unsigned total=0;
  for(unsigned g=0;g<9;++g){auto& group=rig.groups[g];assert(group.targetCount==counts[g]&&group.targets.size()==counts[g]&&!group.state&&!group.timer&&!group.phase);total+=group.targetCount;
   for(unsigned i=0;i<group.targets.size();++i)assert(group.targets[i].m4_pPosition==&group.position&&group.targets[i].radius==0xA000&&group.targets[i].flags==(i?0xF0004002u:0xF0004000u));}
  assert(total==13&&registered==24);
 }
 {s_workAreaCopy owner;LagiLathumRig rig;missingFile=true;registered=0;assert(!lagiInitializeLathumRig(&owner,rig)&&!registered);missingFile=false;}
 {s_workAreaCopy owner;LagiLathumRig rig;failModel=4;registered=0;assert(!lagiInitializeLathumRig(&owner,rig)&&owner.task.finished&&!registered);failModel=~0u;}
 puts("PASS native Lathum initialization: two 21-bone models, 11 core and 13 grouped targets, COMMON health/life binding, RNG selection, formation name and initial mode");
}
