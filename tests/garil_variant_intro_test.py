from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
u=s[s.index('static void BTL_A5_formationVariant_Update'):]
intro=u[u.index('    case 0:'):u.index('    case 3:')]
assert intro.strip()==(r/'src/integration/lagi_garil_variant_intro.inc').read_text().strip()
control=(r/'src/integration/lagi_garil_variant_control.inc').read_text();counter=control[control.index('static bool lagiGarilVariantCounterAfter'):]
fixture=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
using s16=int16_t;using u16=uint16_t;using s32=int32_t;using u32=uint32_t;
struct sVec3_FP{};
struct Formation{struct{sVec3_FP m0_position;}m2C_posBlock;unsigned m28_state=0;s16 m24_counter1=0;};
struct Engine{struct{bool m2000=false,m100_attackAnimationFinished=false;}m188_flags;}engine;
struct File{u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
enum class eBattleModes{m7};bool introFinished=false;unsigned modes=0,cameras=0,speeches=0,restores=0;Formation* expected=nullptr;
bool battleEngine_isBattleIntroFinished(){return introFinished;}
void battleEngine_SetBattleMode(eBattleModes m){assert(m==eBattleModes::m7);++modes;}
void battleEngine_setupAttackCamera(sVec3_FP* p,s32 distance,unsigned mode,s16 duration){assert(p==&expected->m2C_posBlock.m0_position&&distance==0x50000&&!mode&&!duration);++cameras;}
void* lagiCreateNativeBattleSpeech(Formation* p,s16 text,s16 duration,u32 filename){assert(p==expected&&text==17&&duration==15&&filename==0x60AD1A4);++speeches;return nullptr;}
void battleEngine_restoreCameraAfterEnemyAttack(){assert(speeches==1);++restores;}
// COUNTER
void update(Formation* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// INTRO
}}
int main(){
 for(bool finished:{false,true}){Formation f;expected=&f;introFinished=finished;modes=cameras=0;update(&f);assert(f.m28_state==unsigned(finished)&&modes==unsigned(finished)&&cameras==unsigned(finished));}
 for(bool gate:{false,true}){Formation f;expected=&f;f.m28_state=1;engine.m188_flags.m2000=gate;speeches=restores=0;update(&f);assert(f.m28_state==(gate?2:1)&&speeches==unsigned(gate)&&restores==unsigned(gate));}
 for(int old=-32768;old<=32767;++old){Formation f;expected=&f;f.m28_state=2;f.m24_counter1=s16(old);engine.m188_flags.m100_attackAnimationFinished=false;update(&f);const bool done=old>84;assert(f.m28_state==(done?5:2)&&engine.m188_flags.m100_attackAnimationFinished==done);assert(f.m24_counter1==(done?0:s16(u16(old+1))));}
 puts("PASS generated variant intro camera/speech gates, exact shared-service arguments and all signed timer values");
}
'''.replace('// COUNTER',counter).replace('// INTRO',intro)
# initializer-list range loops require this standard header explicitly.
fixture='#include <initializer_list>\n'+fixture
with tempfile.TemporaryDirectory(prefix='lagi-variant-intro-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value,n in [(0x6054A50,0x50000,4),(0x6054A54,0x6061F88,4),(0x6054A6E,0xE60F,2),(0x6054A72,0xE511,2),(0x6054B84,0x60AD1A4,4),(0x6054B88,0x6070E0E,4),(0x6054B8C,0x60818D0,4),(0x6054A8A,0xE354,2),(0x6070E1A,0xE0FF,2)]:
 assert int.from_bytes(data[ea-base:ea-base+n],'big')==value,f'Native intro contract changed at {ea:08X}'
assert data[0x60AD1A4-base:0x60AD1A4-base+12]==b'B_A5_07.PCM\0'
assert 'if (gBattleManager->m6_subBattleId == 9)' in s
print('PASS original Saturn camera/speech factories, authored PCM, timer and encounter gate')
