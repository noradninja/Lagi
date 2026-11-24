from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=(r/'tests/nanyd_motion_test.cpp').read_text().split('static sVec3_FP headAt',1)[0]
f=f.replace('using s16=', 'using u16=uint16_t;using s16=')
f=f.replace('struct Group{','struct sBTL_A5_Enemy;using p_workArea=sBTL_A5_Enemy*;\nstruct Group{int mE_numEntities=10;p_workArea* m0_entityArray=nullptr;')
f=f.replace('bool finished{};','bool finished{};bool isFinished(){return finished;}')
f=f.replace('// PRODUCTION_TRAIL',(r/'src/integration/lagi_nanyd_formation_trail.inc').read_text()).replace('// PRODUCTION_MOTION',(r/'src/integration/lagi_nanyd_motion.inc').read_text())
s=(r/'tests/nanyd_attack_cases.inc').read_text().split('static void same',1)[0]
s=s.replace('struct Overlay{','struct Engine;Engine* enginePointer;struct Overlay{Engine* m4_battleEngine=nullptr;')
f+=s.replace('// PRODUCTION_ATTACKS',(r/'src/integration/lagi_nanyd_attacks.inc').read_text())
f+=r"""
#include "lagi_formation_actor_signal.h"
struct Turn{int m8=1,m0=1;}turn;
struct Engine{Turn* m3CC=&turn;struct{bool m2000=true,m100_attackAnimationFinished=false;}m188_flags;}engine;
struct Dual:sBTL_A5_FormationTask{int m28_state=2;s16 m24_counter1=0;u8 m27C_variant=1;};
struct File{sSaturnPtr getSaturnPtr(u32 ea){return {ea};}}file;auto* g_BTL_A5=&file;
u32& lagiA5ActorFlags(p_workArea actor){return actor->m14_flags;}
int names=0;void battleEngine_displayAttackName(int n,int duration,int flags){assert((n==1&&duration==30)||(n==2&&duration==35));assert(!flags);++names;}
// TIMER
void formationStep(Dual* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PHASES
 default:break;
}}
int main(int argc,char** argv){assert(argc==2);std::ifstream stream(argv[1],std::ios::binary);prg.assign(std::istreambuf_iterator<char>(stream),{});assert(!prg.empty());
 for(int mask=1;mask<1024;++mask)for(bool coordinated:{false,true})for(bool contact:{false,true}){
  Dual f;f.m28_state=coordinated?4:2;f.mE0_entityGroup.m8_dataTable={coordinated?0x60B1330u:0x60B1240u};f.m1CC_dataTable2={0x60B13A8};
  std::array<sBTL_A5_Enemy,10> actors;std::array<p_workArea,10> pointers;
  for(int i=0;i<10;++i){pointers[i]=(mask&(1<<i))?&actors[i]:nullptr;actors[i].m8_formation=&f;actors[i].m1C_entityIndex=i;}
  f.mE0_entityGroup.m0_entityArray=pointers.data();
  const int selected=lagiReadyFormationActors(f.mE0_entityGroup,coordinated?10:3,0x08000000u,coordinated?0x0C000000u:0x04000000u,lagiA5ActorFlags);
  int ordinal=0;for(int i=0;i<10;++i)if(pointers[i]){lagiNanydEnterPendingAttack(&actors[i]);assert(actors[i].m28_state==(ordinal++<selected?(coordinated?2:1):0));}
  names=damages=contacts=randomCalls=animations=0;randoms.clear();sounds.clear();hit=contact;engine.m188_flags.m100_attackAnimationFinished=false;dragon.m8C.m8=&dragon.m8_position;
  formationStep(&f);assert(names==1&&f.m28_state==(coordinated?5:3));
  for(int tick=0;tick<95;++tick){
   if(tick)formationStep(&f);
   for(int i=0;i<10;++i)if(pointers[i]){if(!lagiUpdateNanydAttackState(&actors[i]))lagiNanydEnterPendingAttack(&actors[i]);}
  }
  assert(damages==unsigned(contact?selected:0)&&engine.m188_flags.m100_attackAnimationFinished&&f.m28_state==1);
  for(auto& actor:actors)assert(actor.m28_state==0&&!actor.getTask()->isFinished());
  if(coordinated)assert(f.mE0_entityGroup.m8_dataTable.address==0x60B1240);
 }
 puts("PASS 4092 dual swarm membership/style/contact cases: shared ready/release into production actor attack states, one dragon contact per selected actor and native completion");
}
"""
f=f.replace('// TIMER',(r/'src/integration/lagi_garil_dual_control.inc').read_text().split('void battleEngine_PlayAttackCamera(int);')[0])
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text();f=f.replace('// PHASES',s[s.index('    case 2:'):s.index('    case 6:')])
with tempfile.TemporaryDirectory(prefix='lagi-dual-swarm-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe'),sys.argv[1]],check=True)
