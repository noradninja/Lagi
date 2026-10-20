#include <array>
#include <vector>
#include <cstdint>
#include <cassert>
#include <cstdio>
using s32=int;using s16=std::int16_t;using u16=std::uint16_t;
using Vec=std::array<int,3>;
struct sProcessed3dModel {
 struct Extra {std::array<s16,3> m0_normals{};std::array<u16,3> m6_colors{};};
 struct Quad {u16 m8_lightingControl=0x200;std::vector<Extra> m14_extraData{Extra{}};};
 int m0_radius=0;unsigned m4_numVertices=1;
 std::vector<std::array<s16,3>> m8_vertices{{0,0,0}};
 std::vector<Quad> mC_Quads{Quad{}};
 bool m_vertexBuffersDirty=false;
};
struct sModelHierarchy {sProcessed3dModel* m0_3dModel{};sModelHierarchy *m4_subNode{},*m8_nextNode{};};
struct Pose {struct Bone {Vec m0_translation{};};std::vector<Bone> m0_bones{3};};
sProcessed3dModel dst,srcA,srcB; sModelHierarchy hd{&dst},ha{&srcA},hb{&srcB};Pose pa,pb;
struct Bundle {
 sModelHierarchy* getModelHierarchy(unsigned index){assert(index==8||index==12||index==16);return index==8?&hd:index==12?&ha:&hb;}
 Pose* getStaticPose(unsigned index,unsigned bones){assert(bones==3);assert(index==0x1C0||index==0x1C4);return index==0x1C0?&pa:&pb;}
}bundle;
struct npcFileDeleter {Bundle* m0_fileBundle=&bundle;}owner;
struct s_3dModel {Bundle* m4_pModelFile=&bundle;u16 mC_modelIndexOffset=8,m12_numBones=3;std::vector<Pose::Bone> m2C_poseData{3};};
using sSaturnPtr=unsigned;
u16 readSaturnU16(sSaturnPtr p){switch(p){case 0x060A9538:return 12;case 0x060A953A:return 0x1C0;case 0x060A9540:return 16;case 0x060A9542:return 0x1C4;default:assert(false);return 0;}}
int invalidations=0;
namespace lagi::azel_bridge {void retire_native_model(sProcessed3dModel* p){assert(p==&dst);++invalidations;}}
#include "production.inc"
int main(){
 srcA.m0_radius=100;srcB.m0_radius=300;
 srcA.m8_vertices[0]={-100,200,-300};srcB.m8_vertices[0]={100,-200,300};
 srcA.mC_Quads[0].m14_extraData[0].m0_normals={-100,200,-300};
 srcB.mC_Quads[0].m14_extraData[0].m0_normals={100,-200,300};
 srcA.mC_Quads[0].m14_extraData[0].m6_colors={100,200,300};
 srcB.mC_Quads[0].m14_extraData[0].m6_colors={300,400,500};
 pa.m0_bones[1].m0_translation={100,200,300};pb.m0_bones[1].m0_translation={300,400,500};
 s_3dModel model;model.m2C_poseData[0].m0_translation={9,8,7};
 for(int weight:{0,0x8000,0x10000}){
  lagiNativeTwoPoseMorph(&owner,&model,0x060A9538,0x060A9540,weight);
  assert(dst.m0_radius==lagiMorphPair32(100,300,weight));
  assert(dst.m8_vertices[0][0]==lagiMorphPair16(-100,100,weight));
  assert(dst.mC_Quads[0].m14_extraData[0].m6_colors[0]==lagiMorphPair16(100,300,weight));
  assert(model.m2C_poseData[1].m0_translation[0]==lagiMorphPair32(100,300,weight));
  assert(model.m2C_poseData[0].m0_translation==Vec({9,8,7}));
 }
 assert(invalidations==3&&dst.m_vertexBuffersDirty);
 dst.m_vertexBuffersDirty=false;
 lagiNativeTwoPoseMorph(&owner,&model,0x060A9538,0x060A9540,0x10000);
 assert(invalidations==3&&!dst.m_vertexBuffersDirty);
 // Compare actual output, not a cached weight: externally changed geometry
 // must be restored even if the morph inputs have not changed.
 dst.m8_vertices[0][0]=123;
 lagiNativeTwoPoseMorph(&owner,&model,0x060A9538,0x060A9540,0x10000);
 assert(invalidations==4&&dst.m8_vertices[0][0]==-100);
 lagiNativeTwoPoseMorph(&owner,&model,0x060A9538,0x060A9540,-1);
 lagiNativeTwoPoseMorph(&owner,&model,0x060A9538,0x060A9540,0x10001);
 assert(invalidations==4);
 assert(lagiMorphPair16(32767,-32768,0x8001)==1);
 puts("PASS native two-source mesh/normal/color/pose morph, endpoints, root preservation, signed weight truncation and snapshot invalidation; device pending");
}
