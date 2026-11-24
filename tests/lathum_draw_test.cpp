#include "lathum_rig_fixture.h"
#include "flatten.inc"
struct sMatrix4x3 {int identity;};
struct {sMatrix4x3 m28[2]{{17},{23}};} cameraProperties2;
struct Frame {sVec3_FP origin,rotation;int yaw=0;};
std::vector<Frame> stack(1);
struct Submission {unsigned key;Frame frame;};
std::vector<Submission> submissions;
int normalDraws=0,emergenceDraws=0,transforms=0;
sVec3_FP expectedBodyPosition;
bool same(const sVec3_FP& a,const sVec3_FP& b){return a.x.value==b.x.value&&a.y.value==b.y.value&&a.z.value==b.z.value;}
sVec3_FP world(const sVec3_FP& p){return {p.y.value+0x12345,-p.x.value-0x23456,p.z.value+0x34567};}
void transformAndAddVec(const sVec3_FP& p,sVec3_FP& out,const sMatrix4x3& matrix){assert(&matrix==&cameraProperties2.m28[0]);out=world(p);++transforms;}
void pushCurrentMatrix(){stack.push_back(stack.back());}
void popMatrix(){assert(stack.size()>1);stack.pop_back();}
void translateCurrentMatrix(const sVec3_FP* p){stack.back().origin=*p;}
void rotateCurrentMatrixShiftedY(fixedPoint angle){assert(angle.value==0x08000000);stack.back().yaw=angle.value;}
void rotateCurrentMatrixZYX(const sVec3_FP* p){stack.back().rotation=*p;}
void addObjectToDrawList(sProcessed3dModel* model){assert(stack.size()==3);submissions.push_back({model->key,stack.back()});}
sVec3_FP hotpoint(unsigned index){return {s32(0x10000+index*0x1111),s32(-0x20000-index*0x2222),s32(0x30000+index*0x3333)};}
void drawNormal(s_3dModel* model){
 assert(model->key==4&&stack.size()==2&&stack.back().yaw==0x08000000);++normalDraws;assert(same(stack.back().origin,expectedBodyPosition));
 unsigned index=0;for(auto& bone:model->m44_hotpointData)for(auto& p:bone)p=hotpoint(index++);assert(index==11);
}
void drawEmergence(s_3dModel* model){assert(model->key==12&&stack.size()==2&&stack.back().yaw==0x08000000);++emergenceDraws;assert(same(stack.back().origin,expectedBodyPosition));}
#include "draw.inc"
int main(int argc,char**argv){
 assert(argc==4);original.bytes=load(argv[1]);bundle.bytes=load(argv[2]);common=load(argv[3]);
 s_workAreaCopy owner;LagiLathumRig rig;assert(lagiInitializeLathumRig(&owner,rig));
 rig.models[0].m18_drawFunction=drawNormal;rig.emergenceModel.m18_drawFunction=drawEmergence;
 for(unsigned bone=0;bone<21;++bone)rig.models[0].m2C_poseData[bone].mC_rotation={s32(10+bone),s32(20+bone*2),s32(30+bone*3)};
 rig.position={0x180000,0x38000,-0x68000};expectedBodyPosition=rig.position;
 const sVec3_FP untouched{123,456,789};
 for(auto& p:rig.corePositions)p=untouched;
 for(auto& group:rig.groups)group.position=untouched;
 for(unsigned hide=0;hide<2;++hide){rig.modelPhase=0;rig.hideBody=hide;lagiDrawLathumRig(&owner,rig);}
 assert(emergenceDraws==2&&!normalDraws&&submissions.empty()&&!transforms&&stack.size()==1);
 for(auto& p:rig.corePositions)assert(same(p,untouched));
 const std::array<unsigned,9> anchors{8,3,9,10,6,0,4,5,7},bones{15,4,16,19,9,2,5,8,12};
 const std::array<std::array<unsigned,3>,9> meshes{{{0xB0,0xB4,0xB8},{0x98,0x9C,0xA0},{0x7C,0x80,0x84},{0x70,0x74,0x78},{0x64,0x68,0x6C},{0x88,0x8C,0x90},{0xCC,0xD0,0xD4},{0xA4,0xA8,0xAC},{0xC0,0xC4,0xC8}}};
 for(unsigned combination=0;combination<19683;++combination){
  unsigned value=combination;for(auto& group:rig.groups){group.state=value%3;value/=3;}
  rig.modelPhase=1;rig.hideBody=0;submissions.clear();transforms=0;
  lagiDrawLathumRig(&owner,rig);assert(submissions.size()==11&&transforms==11&&stack.size()==1);
  for(unsigned i=0;i<9;++i){const auto& s=submissions[i];assert(s.key==meshes[i][rig.groups[i].state]);assert(same(s.frame.origin,world(hotpoint(anchors[i]))));assert(same(s.frame.rotation,rig.models[0].m2C_poseData[bones[i]].mC_rotation));}
  for(unsigned i=0;i<2;++i){const auto& s=submissions[9+i];assert(s.key==(i?0xBCu:0x94u));assert(same(s.frame.origin,world(hotpoint(i+1))));assert(same(s.frame.rotation,rig.models[0].m2C_poseData[3].mC_rotation));}
  for(unsigned i=0;i<11;++i)assert(same(rig.corePositions[i],hotpoint(i)));
  for(auto& g:rig.groups)assert(same(g.position,untouched)); // Draw never overwrites live group target positions.
 }
 for(unsigned phase:{1u,2u,3u,255u}){
  rig.modelPhase=phase;rig.hideBody=1;submissions.clear();transforms=0;auto count=normalDraws;lagiDrawLathumRig(&owner,rig);assert(normalDraws==count&&submissions.empty()&&!transforms);
  rig.hideBody=0;lagiDrawLathumRig(&owner,rig);assert(normalDraws==count+1&&submissions.size()==11);
 }
 assert(stack.size()==1);
 puts("PASS native Lathum drawing: emergence/body guards, all 19,683 part-state combinations, 11 live anchors/poses, camera transform and balanced matrix submissions");
}
