from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text();phases=s[s.index('    case 6:'):s.index('    case 10:')]
f=(r/'tests/garil_variant_burst_test.cpp').read_text();f=f[:f.index('struct s_workAreaCopy')]
f+=r'''
#include "lagi_formation_actor_signal.h"
struct Actor{u32 flags=0;bool finished=false;Actor* getTask(){return this;}bool isFinished(){return finished;}};using p_workArea=Actor*;
struct Group{int mE_numEntities=2;Actor** m0_entityArray=nullptr;};
struct Formation{Group m1D0_entityGroup2;unsigned m28_state=0;int m280_phase=0;s16 m24_counter1=0;struct{sVec3_FP m0_position;}m2C_posBlock;sVec3_FP mA8_position,mC0_position2;};
struct Turn{int m8=1,m0=7;}turn;
struct Engine{Turn* m3CC=&turn;struct{bool m2000=false,m100_attackAnimationFinished=false;}m188_flags;}engine;
struct Dragon{sVec3_FP mF0={11,22,33};}dragon;struct Overlay{Dragon* m18_dragon=&dragon;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
unsigned names=0,defaults=0,restores=0;bool player=false;std::vector<unsigned> order;sVec3_FP* eye=nullptr; sVec3_FP* target=nullptr;
u32& lagiA5ActorFlags(p_workArea a){return a->flags;}u32& lagiGarilSingleActorFlags(p_workArea a){return a->flags;}
void battleEngine_displayAttackName(int name,int duration,int flags){assert(name==8&&duration==35&&!flags);++names;}
void battleEngine_enableAttackCamera(){order.push_back(1);}void battleEngine_setDesiredCameraPositionPointer(sVec3_FP* p){target=p;order.push_back(2);}void battleEngine_setCurrentCameraPositionPointer(sVec3_FP* p){eye=p;order.push_back(3);}void battleEngine_resetCameraInterpolation(){order.push_back(4);}
void battleEngine_restoreCameraDefault(){++defaults;}void battleEngine_restoreCameraAfterEnemyAttack(){++restores;}bool battleEngine_isPlayerTurnActive(){return player;}
// CONTROL
void update(Formation* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PHASES
}}
int main(){
 for(bool gate:{false,true}){Formation f;f.m28_state=6;engine.m188_flags.m2000=gate;names=0;update(&f);assert(f.m28_state==(gate?7:6)&&names==unsigned(gate));}
 for(int timer=-32768;timer<=32767;++timer){Formation f;std::array<Actor,2> actors={Actor{0x04001000},Actor{0x400}};std::array<Actor*,2> ptrs={&actors[0],&actors[1]};f.m1D0_entityGroup2.m0_entityArray=ptrs.data();f.m28_state=7;f.m24_counter1=s16(timer);f.m2C_posBlock.m0_position={INT32_MAX,-44444,INT32_MIN};order.clear();update(&f);const bool done=timer>45;assert(f.m28_state==(done?8:7)&&f.m24_counter1==(done?0:s16(u16(timer+1))));assert(f.m280_phase==int(done));if(done){assert(order==std::vector<unsigned>({1,2,3,4}));assert(eye==&f.mA8_position&&target==&f.mC0_position2);assert(f.mC0_position2[2].asS32()==33);assert(u32(f.mA8_position[0].asS32())==u32(INT32_MAX)+0xA000u);assert(f.mA8_position[1].asS32()==-44444-0x1000);assert(u32(f.mA8_position[2].asS32())==u32(INT32_MIN)-0x14000u);assert(actors[0].flags==0x02001000&&actors[1].flags==0x400);}else assert(order.empty()&&actors[0].flags==0x04001000);}
 for(int timer=-32768;timer<=32767;++timer){Formation f;f.m28_state=8;f.m24_counter1=s16(timer);defaults=0;engine.m188_flags.m100_attackAnimationFinished=false;update(&f);const bool done=timer>27;assert(f.m28_state==(done?9:8)&&defaults==unsigned(done)&&engine.m188_flags.m100_attackAnimationFinished==done);}
 for(bool active:{false,true}){Formation f;f.m28_state=9;f.m280_phase=1;turn={1,7};player=active;restores=0;update(&f);assert(f.m28_state==(active?9:0)&&f.m280_phase==int(active)&&turn.m8==int(active)&&turn.m0==(active?7:0)&&restores==unsigned(!active));}
 puts("PASS generated dual secondary phases: camera order/values/wrap, release flags, all signed timing values and turn/camera restoration");
}
'''
f=f.replace('// CONTROL',(r/'src/integration/lagi_garil_dual_control.inc').read_text().split('void battleEngine_PlayAttackCamera(int);')[0]).replace('// PHASES',phases)
with tempfile.TemporaryDirectory(prefix='lagi-dual-secondary-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'include'),'/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F750,0xE408,2),(0x605F74C,0xE523,2),(0x605F75E,0xE32D,2),(0x605F7EA,0xE31B,2),(0x605F886,0xF000,2),(0x605F894,0xA000,4),(0x605F898,0x14000,4),(0x605F89C,0x60A27DC,4),(0x605F8A0,0x60817B4,4),(0x60A2970,0x60818D0,4)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Original dual secondary phase contract changed at {ea:08X}'
print('PASS original secondary camera helpers, signed offset, names and timer operands')
