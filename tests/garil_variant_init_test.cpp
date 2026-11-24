#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
#include "lagi/native_battle_models.h"
#include "lagi/garil_single_formation.h"
struct Task{bool done=false;void markFinished(){done=true;}};
struct s_workAreaCopy{Task task;Task* getTask(){return &task;}};
struct sSaturnPtr{std::uint32_t ea;};
struct sVec3_FP{};struct sBattleTargetable{};
struct Animation{unsigned m4_numFrames=12;};
struct s_3dModel{Animation* m30_pCurrentAnimation=nullptr;int m52_targetableCount=0;};
struct Hierarchy{unsigned countNumberOfBones(){return 3;}};
struct s_fileBundle{
 Animation animation;Hierarchy h;
 Animation* getAnimation(unsigned n){assert(n==0x48);return &animation;}
 Hierarchy* getModelHierarchy(unsigned n){assert(n==8);return &h;}
 int* getStaticPose(unsigned n,unsigned bones){assert(n==0x44&&bones==3);static int p;return &p;}
};
struct Life:s_workAreaCopy{};
struct sBTL_A5_VariantEnemy:s_workAreaCopy{
 std::uint8_t m29_modelCount=0;std::vector<s_3dModel>m2C_models;
 Life* mAC_lifeMeter; s_3dModel m160_collisionModel;
 int m1C_entityIndex=4,mB6_numTargetables=0;LagiBattleModelTargetState m_lagiTargets;
};
struct Data{sSaturnPtr getSaturnPtr(unsigned ea){return {ea};}}data,*g_BTL_A5=&data;
int fail=0,steps=0,targets=0;unsigned expectedVariant;
bool lagiCreateBattleEnemyModels(s_workAreaCopy* owner,s_fileBundle*,std::uint8_t& count,
 std::vector<s_3dModel>& m,sSaturnPtr p,std::uint8_t n){assert(p.ea==0x60AD5C0&&n==1);
 count=1;m.resize(1);if(fail==1){owner->getTask()->markFinished();return false;}return true;}
void initAnimation(s_3dModel* m,Animation* a){m->m30_pCurrentAnimation=a;}
unsigned randomNumber(){return 21;}unsigned performModulo2(unsigned n,unsigned r){assert(n==11&&r==21);return r%n;}
void setAnimationFrame(s_3dModel*,unsigned f){assert(f==10);}void stepAnimation(s_3dModel*){++steps;}
bool init3DModelRawData(s_workAreaCopy*,s_3dModel*,int z,s_fileBundle*,unsigned model,void* anim,int* pose,void* col,void* hp){assert(z==0&&model==8&&!anim&&pose&&!col&&!hp);return fail!=2;}
bool lagiInitBattleEnemyModelTargets(s_workAreaCopy* owner,std::uint32_t index,
 std::vector<s_3dModel>& m,LagiBattleModelTargetState&,const std::array<std::uint32_t,4>& callbacks){
 ++targets;assert(index==4&&(callbacks==std::array<std::uint32_t,4>{0x60A3A4C,0x60A3A50,0x60A3A74,0x60A3A98}));
 if(fail==3){owner->getTask()->markFinished();return false;}m[0].m52_targetableCount=2;return true;}
// PRODUCTION_INIT
int main(){
 for(expectedVariant=0;expectedVariant<2;++expectedVariant)for(fail=0;fail<4;++fail){
  Life life;sBTL_A5_VariantEnemy actor;actor.mAC_lifeMeter=&life;s_fileBundle bundle;
  const bool ok=lagiInitGarilVariantModels(&actor,&bundle);
  assert(ok==(fail==0));assert(life.task.done==!ok);assert(actor.task.done==!ok);
  if(ok){assert(actor.m29_modelCount==1&&actor.mB6_numTargetables==2);assert((actor.m_lagiTargets.hitboxValues==std::array<std::int16_t,4>{45,5,5,5}));}
 }
 puts("PASS production variant constructor: original model/pose/animation, shared target binding and initialization failures");
}
