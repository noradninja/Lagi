#include "lathum_rig_fixture.h"
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
