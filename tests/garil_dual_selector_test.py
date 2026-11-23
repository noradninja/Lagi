from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
f=r'''
#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include "lagi_formation_actor_signal.h"
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;
struct Actor{u32 flags=0;bool finished=false;Actor* getTask(){return this;}bool isFinished(){return finished;}};using p_workArea=Actor*;
struct Pointer{u32 ea=0;};struct File{Pointer getSaturnPtr(u32 a){return {a};}}file;auto* g_BTL_A5=&file;
struct Group{int mE_numEntities=0;Actor** m0_entityArray=nullptr;Pointer m8_dataTable;};
struct Formation{Group mE0_entityGroup,m1D0_entityGroup2;u8 m28_state=0,m1E3_flag2=0,m_lagiMoveCounter=0,m27C_variant=1;u8 m1E4_pad6[12]{};int m284_allSwarmDead=0,m288_allSingleDead=0;};using sBTL_A5_DualFormation=Formation;
struct Turn{int m8=1,m0=9;}turn;struct Engine{Turn* m3CC=&turn;u8 m22C_dragonCurrentQuadrant=0;}engine;struct Dragon{u32 m1C0_statusModifiers=0;}dragon;struct Overlay{Engine* m4_battleEngine=&engine;Dragon* m18_dragon=&dragon;}overlay;struct Manager{Overlay* m10_battleOverlay=&overlay;}manager;auto* gBattleManager=&manager;
enum class eBattleModes{m7,m9};unsigned rngCalls=0,rng=0,sounds=0,modes=0,quadrantMask=0,evades=0;int moved=0;int modeValue=-1;
u32 randomNumber(){++rngCalls;return rng;}void playSystemSoundEffect(int n){assert(n==0x68);++sounds;}void battleEngine_SetBattleMode(eBattleModes mode){modeValue=mode==eBattleModes::m9?9:7;++modes;}void battleEngine_FlagQuadrantBitForAttack(u16 bits){quadrantMask=bits;}
void battleEngine_initiateEnemyMoveDragon(int direction,s16 duration){assert(duration==60);moved=direction;}void arachnoth_startDragonEvade(s16 duration){assert(duration==60);++evades;}
u32& lagiA5ActorFlags(p_workArea a){return a->flags;}u32& lagiGarilSingleActorFlags(p_workArea a){return a->flags;}
// CONTROL
struct Fixture{Formation f;std::array<Actor,10> swarm;std::array<Actor*,10> sp;std::array<Actor,3> singles;std::array<Actor*,3> lp;Fixture(){for(unsigned i=0;i<10;++i){sp[i]=&swarm[i];swarm[i].flags=0x08001000;}for(unsigned i=0;i<3;++i)lp[i]=&singles[i];f.mE0_entityGroup={10,sp.data(),{}};f.m1D0_entityGroup2={3,lp.data(),{}};turn={1,9};dragon.m1C0_statusModifiers=0;rngCalls=sounds=modes=quadrantMask=evades=0;moved=0;modeValue=-1;}};
int main(){
 for(unsigned q:{1u,3u})for(unsigned mask=0;mask<1024;++mask)for(unsigned counter:{0u,1u,2u,255u}){
  Fixture x;engine.m22C_dragonCurrentQuadrant=q;x.f.m_lagiMoveCounter=counter;rng=1;for(unsigned i=0;i<10;++i)if(!(mask&(1u<<i)))x.sp[i]=nullptr;
  lagiSelectGarilDualTurn(&x.f);const bool cycle=counter==2;const bool attack=!cycle&&mask;assert(x.f.m28_state==(attack?2:1));assert(rngCalls==unsigned(!cycle&&!mask));assert(moved==(attack?0:cycle?(q==1?2:1):2));assert(modes==unsigned(attack)&&quadrantMask==(attack?(q==1?2:8):0));assert(sounds==unsigned(!cycle));
  if(attack){unsigned ordinal=0;for(unsigned i=0;i<10;++i)if(mask&(1u<<i)){assert(x.swarm[i].flags==(0x1000u|(ordinal++<3?0x04000000u:0)));}}
 }
 for(unsigned mask=0;mask<1024;++mask)for(unsigned variant=0;variant<2;++variant){Fixture x;engine.m22C_dragonCurrentQuadrant=2;x.f.m27C_variant=variant;rng=0;for(unsigned i=0;i<10;++i)if(!(mask&(1u<<i)))x.sp[i]=nullptr;lagiSelectGarilDualTurn(&x.f);assert(x.f.m28_state==(mask?4:1)&&evades==unsigned(bool(mask))&&rngCalls==unsigned(!mask));if(mask){assert(quadrantMask==4&&x.f.mE0_entityGroup.m8_dataTable.ea==0x60B12B8+variant*0x78);for(unsigned i=0;i<10;++i)if(mask&(1u<<i))assert(x.swarm[i].flags==0x0C001000);}}
 for(unsigned live=0;live<8;++live)for(unsigned dead=0;dead<8;++dead)for(unsigned busy=0;busy<8;++busy){Fixture x;engine.m22C_dragonCurrentQuadrant=0;x.f.m1E3_flag2=busy;bool eligible=false;for(unsigned i=0;i<3;++i){if(!(live&(1u<<i)))x.lp[i]=nullptr;const s32 word=(dead>>i)&1;std::memcpy(x.f.m1E4_pad6+i*4,&word,4);if((live&(1u<<i))&&!word&&!(busy&(1u<<i)))eligible=true;}lagiSelectGarilDualTurn(&x.f);assert(x.f.m28_state==(eligible?6:0)&&!rngCalls&&!moved);if(eligible){assert(quadrantMask==1&&modes==1&&modeValue==9);for(unsigned i=0;i<3;++i)assert(x.singles[i].flags==(((live&(1u<<i))&&!(dead&(1u<<i)))?0x0C000000u:0));}else assert(!turn.m8&&!turn.m0);}
 for(unsigned q=0;q<4;++q)for(unsigned word:{0u,1u}){Fixture x;engine.m22C_dragonCurrentQuadrant=q;rng=word;x.f.m284_allSwarmDead=x.f.m288_allSingleDead=1;lagiSelectGarilDualTurn(&x.f);assert(x.f.m28_state==1&&moved==int(word+1)&&rngCalls==1&&!turn.m8&&!turn.m0);}
 for(unsigned word:{0u,1u}){Fixture x;engine.m22C_dragonCurrentQuadrant=0;dragon.m1C0_statusModifiers=1;rng=word;lagiSelectGarilDualTurn(&x.f);assert(moved==int(word+1)&&rngCalls==1&&x.f.m28_state==1);}
 puts("PASS production dual selector: exhaustive swarm membership, side cycles, coordinated layouts, secondary dead/busy masks, native RNG fallbacks and status gate");
}
'''.replace('// CONTROL',(r/'src/integration/lagi_garil_dual_control.inc').read_text())
with tempfile.TemporaryDirectory(prefix='lagi-dual-selector-') as d:
 p=Path(d);(p/'test.cpp').write_text(f)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20','/I'+str(r/'src/integration'),str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_dual.cpp').read_text()
assert 'if(pEngine->m3CC->m8==1 && !pThis->m1E3_flag2)lagiSelectGarilDualTurn(pThis);' in s
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x605FD06,0x1C0,2),(0x605FD0A,0x1E4,2),(0x605FD0E,0x1E3,2),(0x605FE36,0x278,2),(0x605FE4A,0xE503,2),(0x605FD52,0xE409,2),(0x605FE38,0x60619CC,4),(0x605FF68,0x60B12B8,4),(0x605FF6C,0x606196C,4)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native dual selector contract changed at {ea:08X}'
print('PASS original dual selector state/status fields, selection count, mode and evade contracts')
