#include "lagi/native_color_animation.h"
#include <array>
#include <vector>
#include <cassert>
#include <cstdio>
using sVec3_FP=std::array<int,3>;
struct Pose {sVec3_FP m0_translation{},mC_rotation{},m18_scale{1,1,1};};
struct Definition {struct Point{sVec3_FP m4;};std::vector<Point> m0;unsigned m4_count=0;};
struct sProcessed3dModel {int id;};
struct sModelHierarchy {sProcessed3dModel* m0_3dModel; sModelHierarchy* m4_subNode; sModelHierarchy* m8_nextNode;};
struct Bundle {sModelHierarchy* root;sModelHierarchy* getModelHierarchy(int){return root;}};
struct s_3dModel {
 unsigned m12_numBones=0,m8=1,mA_animationFlags=0,mC_modelIndexOffset=0;
 int m14=0;std::uint8_t* m38_pColorAnim=nullptr;Bundle* m4_pModelFile=nullptr;
 std::vector<int> m3C_boneMatrices;std::vector<Pose> m2C_poseData;
 std::vector<Definition>* m40=nullptr;std::vector<std::vector<sVec3_FP>> m44_hotpointData;
};
static int depth=0,scaled=0,multiplied=0;static std::vector<int> order;
void pushCurrentMatrix(){++depth;}void popMatrix(){--depth;}
void multiplyCurrentMatrix(int*){++multiplied;}
void translateCurrentMatrix(sVec3_FP*){}void rotateCurrentMatrixZYX(sVec3_FP*){}
void scaleCurrentMatrixRow0(int){++scaled;}void scaleCurrentMatrixRow1(int){++scaled;}void scaleCurrentMatrixRow2(int){++scaled;}
void transformAndAddVecByCurrentMatrix(sVec3_FP* in,sVec3_FP* out){*out=*in;(*out)[0]+=depth;}
void addObjectToDrawListWithColorAnimation(sProcessed3dModel* m,const std::uint8_t* colors){assert(!colors);order.push_back(m->id);}
#include "../src/integration/lagi_native_color_draw.inc"
int main(){
 sProcessed3dModel a{1},b{2},c{3};
 sModelHierarchy third{&c,nullptr,nullptr},child{&b,nullptr,nullptr},root{&a,&child,&third};Bundle bundle{&root};
 s_3dModel model;model.m12_numBones=3;model.m4_pModelFile=&bundle;model.m2C_poseData.resize(3);model.m3C_boneMatrices.resize(3);
 std::vector<Definition> points(3);for(auto& p:points){p.m0.push_back({{4,5,6}});p.m4_count=1;}
 for(bool matrix:{false,true})for(bool scale:{false,true})for(bool hotpoints:{false,true}){
  model.m8=matrix?3:1;model.mA_animationFlags=scale?0x20:0;model.m40=hotpoints?&points:nullptr;
  model.m44_hotpointData.assign(3,std::vector<sVec3_FP>(1));order.clear();scaled=multiplied=0;
  lagiDrawColorAnimatedModel(&model);assert(depth==0 && order==std::vector<int>({1,2,3}));
  assert(multiplied==(matrix?3:0));assert(scaled==(!matrix && scale?9:0));
  if(hotpoints){assert(model.m44_hotpointData[0][0][0]==5);assert(model.m44_hotpointData[1][0][0]==6);assert(model.m44_hotpointData[2][0][0]==5);}
 }
 model.m8=0;order.clear();lagiDrawColorAnimatedModel(&model);assert(order.empty());
 puts("PASS shared colored draw: preorder child/sibling traversal, matrix/pose/scaled paths, hotpoints, balanced stack and original billboard exclusion");
}
