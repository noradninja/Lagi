from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
phases=s[s.index('    case 2:'):s.index('    case 6:')]
assert phases.strip()==(r/'src/integration/lagi_garil_dual_swarm_phases.inc').read_text().strip()
fixture=r'''
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "lagi_formation_actor_signal.h"
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;
struct Actor{bool finished=false;u32 flags=0;Actor* getTask(){return this;}bool isFinished(){return finished;}};using p_workArea=Actor*;
struct Pointer{u32 ea=0;};struct File{Pointer getSaturnPtr(u32 a){return {a};}}file;auto* g_BTL_A5=&file;
struct Group{int mE_numEntities=2;Actor** m0_entityArray=nullptr;Pointer m8_dataTable;};
struct Formation{Group mE0_entityGroup;u8 m28_state=0,m27C_variant=0;s16 m24_counter1=0;};
struct Engine{struct{bool m2000=false,m100_attackAnimationFinished=false;}m188_flags;}engine;
unsigned names=0;int expectedName=0;
u32& lagiA5ActorFlags(p_workArea a){return a->flags;}
void battleEngine_displayAttackName(int name,int duration,int flags){assert(name==expectedName&&duration==(name==1?30:35)&&!flags);++names;}
// CONTROL
void update(Formation* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PHASES
}}
int main(){
 for(unsigned state:{2u,4u})for(bool gate:{false,true})for(unsigned slots=0;slots<9;++slots)for(unsigned bits=0;bits<16;++bits){
  std::array<Actor,2> actors;std::array<Actor*,2> ptrs;unsigned code=slots;
  for(unsigned i=0;i<2;++i){unsigned status=code%3;code/=3;ptrs[i]=status?&actors[i]:nullptr;actors[i].finished=status==2;actors[i].flags=0x1000|((bits&(1u<<(i*2)))?0x04000000:0)|((bits&(2u<<(i*2)))?0x02000000:0);}
  Formation f;f.mE0_entityGroup.m0_entityArray=ptrs.data();f.m28_state=state;engine.m188_flags.m2000=gate;names=0;expectedName=state==2?1:2;const auto old=actors;
  update(&f);assert(f.m28_state==(gate?state+1:state)&&names==unsigned(gate));
  code=slots;for(unsigned i=0;i<2;++i){const unsigned status=code%3;code/=3;const u32 original=old[i].flags;assert(actors[i].flags==(gate&&status==1&&(original&0x04000000)?(original&~0x04000000u)|0x02000000u:original));if(gate&&status!=1)assert(!ptrs[i]);}
 }
 for(unsigned state:{3u,5u})for(unsigned variant=0;variant<2;++variant)for(int timer=-32768;timer<=32767;++timer){Formation f;f.m28_state=state;f.m27C_variant=variant;f.m24_counter1=s16(timer);f.mE0_entityGroup.m8_dataTable={999};engine.m188_flags.m100_attackAnimationFinished=false;update(&f);const bool done=timer>(state==3?15:60);assert(f.m28_state==(done?1:state));assert(f.m24_counter1==(done?0:s16(u16(timer+1))));assert(engine.m188_flags.m100_attackAnimationFinished==done);assert(f.mE0_entityGroup.m8_dataTable.ea==(state==5&&done?0x60B11C8+variant*0x78:999));}
 puts("PASS generated dual swarm phases: readiness, native release masks, null/finished filtering, names/durations, all signed timer values and layout restoration");
}
'''.replace('// CONTROL',(r/'src/integration/lagi_garil_dual_control.inc').read_text()).replace('// PHASES',phases)
with tempfile.TemporaryDirectory(prefix='lagi-dual-phases-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605F66A,0xE401,2),(0x605F666,0xE51E,2),(0x605F6C4,0xE402,2),(0x605F6C0,0xE523,2),(0x605F67E,0xE30F,2),(0x605F6D6,0xE33C,2),(0x605F5C8,0x60A2AB0,4),(0x605F730,0x60B11C8,4)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native dual swarm phase contract changed at {ea:08X}'
print('PASS original dual phase names, timing, shared release helper and restored table base')
