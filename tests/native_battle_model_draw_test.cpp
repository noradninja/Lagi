#define main modelLifecycleFixtureMain
#include "native_battle_models_test.cpp"
#undef main
#include <tuple>
using Event=std::tuple<char,int,int>;
static std::vector<Event> events;
static int depth=0,drawIndex=0,mutation=0,mutationIndex=0;
static uint32_t* liveFlags=nullptr;
static const sVec3_FP* expectedPosition=nullptr;
static const sVec3_FP* expectedRotation=nullptr;
static s_3dModel* firstModel=nullptr;
static void setupConditionalLightColor(int value){events.emplace_back('L',value,depth);}
static void clearLightColor(){events.emplace_back('C',0,depth);}
static void pushCurrentMatrix(){events.emplace_back('P',drawIndex,depth);++depth;}
static void translateCurrentMatrix(const sVec3_FP* p){assert(p==expectedPosition&&depth==1);events.emplace_back('T',p->x,p->y);}
static void rotateCurrentMatrixYXZ(const sVec3_FP* p){assert(p==expectedRotation&&depth==1);events.emplace_back('R',p->y,p->x);}
static void draw(s_3dModel* p){
 assert(depth==1&&p==firstModel+drawIndex);events.emplace_back('D',drawIndex,depth);
 if(drawIndex==mutationIndex){
  if(mutation==1)*liveFlags|=0x800000u;
  if(mutation==2)*liveFlags&=~0x800000u;
  if(mutation==3)*liveFlags^=0x800000u;
 }
}
static void popMatrix(){assert(depth==1);--depth;events.emplace_back('O',drawIndex,depth);++drawIndex;}
#include "../src/integration/lagi_battle_model_draw.inc"
int main(){
 sVec3_FP position{123456,-654321,0x7fffffff},rotation{-0x08000000,0x0fffffff,0x01234567};
 expectedPosition=&position;expectedRotation=&rotation;
 const int16_t colors[]={-32768,-1,0,1,10,255,32767};
 size_t cases=0;
 for(unsigned mask=0;mask<32;++mask)for(unsigned count=0;count<6;++count)for(auto color:colors)for(int change=0;change<4;++change)for(int at=0;at<2;++at){
  uint32_t flags=(mask&1?0x800000u:0)|(mask&2?0x80000000u:0)|(mask&4?0x10000000u:0)|(mask&8?0x40000000u:0)|(mask&16?0x44u:0);
  auto expectedFlags=flags;
  liveFlags=&flags;mutation=change;mutationIndex=at?int(count)-1:0;
  std::vector<s_3dModel> models(count);for(auto& m:models)m.m18_drawFunction=draw;
  firstModel=models.data();depth=drawIndex=0;events.clear();
  std::vector<Event> expected;
  if(expectedFlags&0x800000u)expected.emplace_back('L',color,0);
  for(unsigned i=0;i<count;++i){
   expected.emplace_back('P',i,0);expected.emplace_back('T',position.x,position.y);
   expected.emplace_back('R',rotation.y,rotation.x);expected.emplace_back('D',i,1);
   if(int(i)==mutationIndex){if(change==1)expectedFlags|=0x800000u;if(change==2)expectedFlags&=~0x800000u;if(change==3)expectedFlags^=0x800000u;}
   expected.emplace_back('O',i,0);
  }
  if(expectedFlags&0x800000u){expectedFlags&=~0x800000u;expected.emplace_back('C',0,0);}
  lagiDrawBattleEnemyModels(flags,color,position,rotation,models);
  assert(events==expected&&flags==expectedFlags&&depth==0&&drawIndex==int(count));++cases;
 }
 std::printf("PASS: %zu actual native draw service matrix/callback/color-order cases; mocked engine services, not GPU pixels\n",cases);
}
