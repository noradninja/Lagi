#include "lagi_battle_enemy_lifecycle.h"
struct Dragon{sVec3_FP m8_position;sBattleTargetable m8C;}dragon;
struct Overlay{Dragon* m18_dragon=&dragon;}overlay;
struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;
auto* gBattleManager=&manager;
unsigned animations{},randomCalls{},contacts{},lookCalls{},damages{};
bool hit{};std::vector<u32> randoms;std::vector<int> sounds;
sVec3_FP lastLookAt,contactSource,contactTarget,damagePosition,damageImpulse;
s32 contactRadius{},targetRadius{},damageValue{},damageKind{},damageFlags{};
void stepAnimation(Model*){++animations;}
u32 randomNumber(){++randomCalls;if(randoms.empty())return 128;u32 n=randoms.front();randoms.erase(randoms.begin());return n;}
void computeLookAt(const sVec3_FP& delta,sVec2_FP& a){lastLookAt=delta;++lookCalls;a[0]=0x11110000;a[1]=0x22220000;}
fixedPoint getCos(s32 a){assert(a==0x222);return 0x10000;}
fixedPoint getSin(s32 a){assert(a==0x222||a==0x111);return a==0x222?0x8000:-0x10000;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return lagi::formationMultiply(a.asS32(),b.asS32());}
bool lagiBattleContactPoint(const sVec3_FP& p,s32 r,const sVec3_FP& t,s32 tr,sVec3_FP& out){
 ++contacts;contactSource=p;contactTarget=t;contactRadius=r;targetRadius=tr;out[0]=0xBAD;return hit;}
void playSystemSoundEffect(int n){sounds.push_back(n);}
void applyDamageToDragon(sBattleTargetable& t,s32 damage,sVec3_FP& p,s32 kind,const sVec3_FP& impulse,s32 flags){
 assert(&t==&dragon.m8C);++damages;damageValue=damage;damageKind=kind;damagePosition=p;damageImpulse=impulse;damageFlags=flags;}
// PRODUCTION_ATTACKS
static void same(const sVec3_FP& a,const sVec3_FP& b){for(unsigned i=0;i<3;++i)assert(a[i].asS32()==b[i].asS32());}
struct Fixture{
 sBTL_A5_FormationTask formation;sBTL_A5_Enemy actor;FakeMeter meter;sVec3_FP target;
 Fixture(){formation.mE0_entityGroup.m8_dataTable={0x060B0D24};formation.m1CC_dataTable2={0x060B0F04};
  actor.m8_formation=&formation;actor.mAC_lifeMeter=&meter;actor.m1C_entityIndex=0;
  target[0]=900000;target[1]=200000;target[2]=-500000;dragon.m8C.m8=&target;
  dragon.m8_position[0]=800000;dragon.m8_position[1]=100000;dragon.m8_position[2]=-400000;
  hit=false;randomCalls=animations=contacts=lookCalls=damages=0;randoms.clear();sounds.clear();}
};
int main(int argc,char** argv){assert(argc==2);std::ifstream stream(argv[1],std::ios::binary);assert(stream);prg.assign(std::istreambuf_iterator<char>(stream),{});
 for(u32 flags:{0u,0x04000000u,0x0C000000u}){Fixture x;x.actor.m14_flags=flags;
  assert(!lagiUpdateNanydAttackState(&x.actor));lagiNanydEnterPendingAttack(&x.actor);
  assert(x.actor.m28_state==(flags==0?0:flags==0x04000000u?1:2));assert(animations==0&&randomCalls==0);
 }
 {Fixture x;x.actor.m28_state=6;x.actor.m14_flags=0x0C000000u;
  lagiNanydEnterPendingAttack(&x.actor);assert(x.actor.m28_state==6&&!lagiUpdateNanydAttackState(&x.actor));}
 {
  Fixture x;x.actor.m28_state=1;
  assert(lagiUpdateNanydAttackState(&x.actor)&&x.actor.m28_state==1&&animations==2&&lookCalls==1);
  x.actor.m14_flags=0x02000000u;assert(lagiUpdateNanydAttackState(&x.actor));
  assert(x.actor.m28_state==3&&animations==4&&randomCalls==0&&x.actor.m_lagiRushTarget==&dragon.m8C);
  same(lastLookAt,x.target-x.actor.m30_position);
  assert(x.actor.m_lagiPositionVelocity[0].asS32()==-0x4A12);
  assert(x.actor.m_lagiPositionVelocity[2].asS32()==-0x2509);
  assert(x.actor.m_lagiPositionVelocity[1].asS32()==-0x4A12);
  randoms={0,255,128};const auto before=lookCalls;lagiNanydUpdateRush(&x.actor);
  assert(randomCalls==3&&lookCalls==before);
  assert(x.actor.m_lagiPositionVelocity[0].asS32()==-0x4A12-128);
  assert(x.actor.m_lagiPositionVelocity[2].asS32()==-0x2509+127);
  assert(x.actor.m4C_velocityY==-0x4A12&&x.actor.m24_counter==1);
  assert(contactRadius==0x2000&&targetRadius==0x3000);same(contactTarget,x.target);
 }
 for(bool queenDead:{false,true}){
  Fixture x;lagiNanydPrepareRush(&x.actor);x.actor.m28_state=3;x.actor.m24_counter=12;
  x.formation.mF0_flag=queenDead;hit=true;x.actor.m17C_hitFlashCount=23;
  lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m28_state==0&&x.actor.m24_counter==12&&damages==1&&damageValue==23&&damageKind==1&&damageFlags==0x800);
  same(damagePosition,dragon.m8_position);for(auto v:damageImpulse)assert(v.asS32()==0);
  assert(sounds==std::vector<int>{0x67}&&!x.actor.task.finished&&x.formation.mE0_entityGroup.mF_deadCount==0);
 }
 for(bool queenDead:{false,true}){
  Fixture x;lagiNanydPrepareRush(&x.actor);x.actor.m28_state=3;x.formation.mF0_flag=queenDead;
  x.actor.m24_counter=69;lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m28_state==3&&x.actor.m24_counter==70&&!x.actor.task.finished);
  lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m24_counter==0&&damages==0);
  if(queenDead){assert(x.actor.task.finished&&x.actor.m28_state==3&&x.meter.task.finished&&!x.actor.mAC_lifeMeter);
   assert(x.formation.mE0_entityGroup.mF_deadCount==1&&x.meter.m31==0);}
  else assert(x.actor.m28_state==0&&!x.actor.task.finished&&!x.meter.task.finished&&x.actor.mAC_lifeMeter==&x.meter);
 }
 {
  Fixture x;x.actor.m28_state=2;x.actor.m14_flags=0x0A000000u;
  assert(lagiUpdateNanydAttackState(&x.actor)&&x.actor.m28_state==4&&lookCalls==0&&randomCalls==0);
  hit=true;x.actor.m17E_hitFlashColor=14;lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m28_state==4&&x.actor.m_lagiCoordinatedHit&&damages==1&&damageValue==14&&damageKind==2&&damageFlags==0x800);
  assert(contactRadius==0x14000&&targetRadius==0x3000&&x.actor.m24_counter==1);
  same(damagePosition,dragon.m8_position);const auto before=contacts;
  x.actor.m24_counter=90;lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m28_state==4&&x.actor.m24_counter==91&&contacts==before&&damages==1);
  lagiUpdateNanydAttackState(&x.actor);
  assert(x.actor.m28_state==0&&x.actor.m24_counter==0&&!x.actor.m_lagiCoordinatedHit);
  assert(x.actor.m14_flags==0x02000000u&&randomCalls==0&&animations==8);
 }
 {Fixture x;x.actor.m28_state=5;x.actor.m14_flags=0x48000000u;
  assert(lagiUpdateNanydAttackState(&x.actor)&&x.actor.m28_state==0&&x.actor.m14_flags==0x08000000u&&animations==0);}
 std::puts("PASS native Nanyd actor attack states: charge geometry/RNG, authored damage, one-hit coordination, old-counter completion and queen-loss retirement without rewards");
}
