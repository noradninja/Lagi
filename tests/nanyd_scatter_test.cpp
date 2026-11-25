#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <vector>
using u32=uint32_t;using s32=int32_t;
struct sVec3_FP {int m0_X=0,m4_Y=0,m8_Z=0;
 int& operator[](int i){return i==0?m0_X:i==1?m4_Y:m8_Z;}
 int operator[](int i)const{return i==0?m0_X:i==1?m4_Y:m8_Z;}
 sVec3_FP operator+(sVec3_FP b)const{return {m0_X+b.m0_X,m4_Y+b.m4_Y,m8_Z+b.m8_Z};}
 sVec3_FP operator-(sVec3_FP b)const{return {m0_X-b.m0_X,m4_Y-b.m4_Y,m8_Z-b.m8_Z};}
};
using sVec2_FP=std::array<int,2>;
struct Task {bool done=false;bool isFinished(){return done;}void markFinished(){done=true;}};
struct Actor {Task task;Task* getTask(){return &task;}};
using p_workArea=Actor*;
struct Group {Actor** m0_entityArray;int mE_numEntities;int mF_deadCount=0;int m8_dataTable=0;};
using sEntityGroup=Group;
struct Position {sVec3_FP m0_position;};
struct sBTL_A5_FormationTask {
 Position m2C_posBlock;std::array<sVec3_FP,18>m_lagiNanydHistory{};
 sVec3_FP mC0_position2;int m1D4_flag=0,mF0_flag=0,m28_state=3,m1CC_dataTable2=100;
 Group mE0_entityGroup{};
};
struct Counter {int m8=1,m0=2,m2=3,m4=4;}counter;
struct Flags {bool m2000=false,m10000=false,m100_attackAnimationFinished=false;};
struct s_battleEngine {sVec3_FP mC_battleCenter{100,200,300};Counter* m3CC=&counter;Flags m188_flags;}engine;
struct Dragon{sVec3_FP mF0;}dragon;
struct Overlay{s_battleEngine* m4_battleEngine=&engine;Dragon* m18_dragon=&dragon;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager,*gBattleManager=&manager;
struct s_3dModel{};
struct sBTL_A5_Enemy:Actor {
 u32 m14_flags=0;int m28_state=0,m1C_entityIndex=0,m24_counter=0;
 sBTL_A5_FormationTask* m8_formation;
 sVec3_FP m30_position,m90_initialPosition,m60_targetPosition,m198_orbitVelocity;
 sVec3_FP m3C_rotation,m9C_targetRotation,m84_displayRotation;
 sVec3_FP m_lagiScatterOffset,m_lagiScatterVelocity,m_lagiScatterAcceleration;
 sVec3_FP m_lagiScatterAngularVelocity,m_lagiScatterAngularAcceleration;
 std::vector<s_3dModel>m2C_models{1};
};
#include "../include/lagi/native_battle_models.h"
#include "../src/integration/lagi_battle_formation_signal.h"
#include "../include/lagi/native_formation_motion.h"
int cleanups=0,animationSteps=0,motionSteps=0,modeChanges=0,labels=0;
bool intro=false;
void BTL_A5_enemy_cleanup(sBTL_A5_Enemy* a,void*){++cleanups;a->m14_flags|=0x90000000u;}
sVec3_FP readSaturnVec3(int p){assert(p==0);return {1,2,3};}
unsigned readSaturnU8(int p){assert(p==100);return 1;}
unsigned randomNumber(){return 0x10000;}
unsigned performModulo2(unsigned a,unsigned b){return b%a;}
int MTH_Mul(int a,int b){return lagi::formationMultiply(a,b);}
void stepAnimation(s_3dModel*){++animationSteps;}
void computeLookAt(const sVec3_FP&,sVec2_FP& a){a={17,19};}
void lagiUpdateBattleModelSpringMotion(const LagiBattleModelMotionView& v){
 ++motionSteps;assert(v.parentPosition.m0_X==100);
 // Arithmetic itself is exercised by native_formation_motion_test.
 for(int i=0;i<3;++i){lagi::NativeFormationSpringAxis p{v.targetPosition[i],v.currentPosition[i],v.velocity[i],v.acceleration[i],v.worldPosition[i]};
 lagi::stepNativeFormationSpring(p,v.parentPosition[i],0xff,0xfff);
 v.currentPosition[i]=p.current;v.velocity[i]=p.velocity;v.acceleration[i]=p.acceleration;v.worldPosition[i]=p.world;}
}
#include "../src/integration/lagi_nanyd_scatter_actor.inc"
void formationUpdatePosition(Position* p,sVec3_FP* center){p->m0_position=*center;}
void formationApplyAutoScroll(sBTL_A5_FormationTask*){}
bool battleEngine_isBattleIntroFinished(){return intro;}
namespace eBattleModes {constexpr int m7=7;}
void battleEngine_SetBattleMode(int m){assert(m==7);++modeChanges;engine.m188_flags.m2000=false;}
void battleEngine_displayAttackName(int a,int b,int c){assert(a==3&&b==0x28&&c==0);++labels;}
#include "../src/integration/lagi_nanyd_scatter_formation.inc"
int main(){
 sBTL_A5_FormationTask f;sBTL_A5_Enemy live,dead;live.m8_formation=&f;dead.m8_formation=&f;
 dead.task.done=true;Actor* actors[]={&live,&dead,nullptr};f.mE0_entityGroup={actors,3};
 assert(!lagiUpdateNanydScatterFormation(&f,&engine));assert(modeChanges==0);
 f.mF0_flag=1;assert(!lagiUpdateNanydScatterFormation(&f,&engine));assert(modeChanges==0);
 intro=true;assert(lagiUpdateNanydScatterFormation(&f,&engine));
 assert(modeChanges==1&&f.m28_state==9&&f.m1D4_flag==1);
 assert(counter.m8==0&&counter.m0==0&&counter.m2==0&&counter.m4==0);
 assert(lagiUpdateNanydScatterFormation(&f,&engine));assert(labels==0&&live.m14_flags==0);
 engine.m188_flags.m2000=true;assert(lagiUpdateNanydScatterFormation(&f,&engine));
 assert(labels==1&&live.m14_flags==8&&!actors[1]&&!actors[2]);
 assert(f.m28_state==11&&engine.m188_flags.m10000&&engine.m188_flags.m100_attackAnimationFinished);
 assert(f.m_lagiNanydHistory[0].m0_X==100&&f.m_lagiNanydHistory[1].m0_X==100);
 live.m30_position={10,-10,30};assert(lagiUpdateNanydScatterActor(&live));
 assert(live.m28_state==6&&cleanups==1&&animationSteps==0);
 assert(live.m_lagiScatterOffset.m0_X==60&&live.m_lagiScatterOffset.m8_Z==180);
 assert(live.m_lagiScatterOffset.m4_Y==98364); // abs(-10 + 0x10000 - 0x14000)*6
 for(int i=0;i<91;++i){assert(lagiUpdateNanydScatterActor(&live));assert(!live.task.done);}
 assert(lagiUpdateNanydScatterActor(&live));assert(live.task.done&&f.mE0_entityGroup.mF_deadCount==1);
 assert(animationSteps==184&&motionSteps==92&&live.m84_displayRotation.m8_Z==0);
 sBTL_A5_FormationTask allQueens;allQueens.m1D4_flag=1;allQueens.mF0_flag=1;
 assert(!lagiUpdateNanydScatterFormation(&allQueens,&engine));assert(modeChanges==1);
 puts("PASS production Scatter: native gates, battle mode, typed survivor signal, history, escape offset, no death award, timer, special formation exclusion");
}
