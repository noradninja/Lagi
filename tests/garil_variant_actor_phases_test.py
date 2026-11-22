from pathlib import Path
import subprocess,tempfile,sys
r=Path(__file__).resolve().parents[1]
s=(r/'build-vita/azel_upstream/battle_BTL_A5_BTL_A5_variant.cpp').read_text()
u=s[s.index('static void BTL_A5_enemyVariant_Update'):s.index('// 06054df8')]
phases=u[u.index('    case 3:'):u.index('    case 9:')]
fixture=r'''
#include <vector>
#include <array>
#include <cstdint>
#include <cassert>
#include <cstdio>
#include <cstring>
using u8=uint8_t;using u16=uint16_t;using u32=uint32_t;using s16=int16_t;using s32=int32_t;
struct Animation{u16 a=0,b=0,frames=60;}animation;
struct Model{u16 m16_previousAnimationFrame=0;Animation* m30_pCurrentAnimation=&animation;};
struct File{Animation* getAnimation(u16 id){assert(id==0x48||id==0x50);return &animation;}u32 getSaturnPtr(u32 a){return a;}}file;auto* g_BTL_A5=&file;
u16 readSaturnU16(u32 a){assert(a==0x60AD5C0||a==0x60AD5C8);return a==0x60AD5C0?0x100:0x108;}
struct Formation{std::array<u8,3> mF4_entityPositions{};u8 reactions=0;}formation;
struct Actor{std::vector<Model> m2C_models=std::vector<Model>(1);File* m0_fileBundle=&file;u8 m28_state=0;u32 m14_flags=0;Formation* m158_formation=&formation;int m1C_entityIndex=0;};
struct Engine{u8 m22C_dragonCurrentQuadrant=0;}engine;
unsigned frame=0,steps=0,inits=0,fires=0,sounds=0,clears=0;
s16 stepAnimation(Model*){++steps;return s16(frame);}
void initAnimation(Model*,Animation* a){assert(a==&animation);++inits;}
void lagiFireGarilVariantBurst(Actor*){++fires;}
void playSystemSoundEffect(int sound){assert(sound==0x6D);++sounds;}
void lagiSetGarilVariantReaction(Actor* actor,bool active){assert(!active);++clears;formation.reactions&=u8(~(1u<<actor->m1C_entityIndex));}
void update(Actor* pThis){auto* pEngine=&engine;switch(pThis->m28_state){
// PRODUCTION
}}
int main(){
 for(unsigned previous=0;previous<128;++previous)for(u32 flags:{0u,0x04000000u,0x02000000u,0x06000000u}){
  Actor a;a.m28_state=3;a.m14_flags=flags;a.m2C_models[0].m16_previousAnimationFrame=previous;steps=inits=0;update(&a);assert(steps==1);assert(a.m28_state==(previous<2?4:3));assert(inits==unsigned(previous<2));assert(a.m14_flags==flags);
 }
 for(unsigned f=0;f<60;++f){Actor a;a.m28_state=4;frame=f;steps=fires=sounds=0;update(&a);assert(steps==1&&fires==unsigned(f==46)&&sounds==unsigned(f==46));assert(a.m28_state==(f==46?5:4));}
 for(unsigned state:{5u,7u,8u})for(unsigned f=0;f<60;++f){
  Actor a;a.m28_state=state;a.m14_flags=0x1004;a.m1C_entityIndex=1;formation.reactions=7;frame=f;steps=inits=clears=0;update(&a);const bool done=f==59;assert(a.m28_state==(done?0:state));assert(inits==unsigned(done));assert(steps==1+unsigned(done&&state!=5));assert(clears==unsigned(done&&state!=5));assert(a.m14_flags==(done&&state!=5?0x1000:0x1004));assert(formation.reactions==(done&&state!=5?5:7));
 }
 for(unsigned q=0;q<4;++q)for(unsigned f=0;f<60;++f){Actor a;a.m28_state=6;a.m1C_entityIndex=2;formation.mF4_entityPositions[2]=0xFF;engine.m22C_dragonCurrentQuadrant=q;frame=f;update(&a);assert(a.m28_state==(f==43?8:6));assert(formation.mF4_entityPositions[2]==(f==43?q:0xFF));}
 Actor a;a.m28_state=3;a.m2C_models[0].m16_previousAnimationFrame=0;frame=0;update(&a);assert(a.m28_state==4);frame=46;fires=0;update(&a);assert(a.m28_state==5&&fires==1);frame=59;update(&a);assert(a.m28_state==0);
 puts("PASS generated variant actor states 3-8: native frame gates, single launch, reaction completion and no invented release wait");
}
'''.replace('// PRODUCTION',phases)
with tempfile.TemporaryDirectory(prefix='lagi-variant-actor-') as d:
 p=Path(d);(p/'test.cpp').write_text(fixture)
 subprocess.run(['cl','/nologo','/EHsc','/std:c++20',str(p/'test.cpp'),'/Fe:'+str(p/'test.exe'),'/Fo:'+str(p/'test.obj')],check=True)
 subprocess.run([str(p/'test.exe')],check=True)
data=Path(sys.argv[1]).read_bytes();base=0x6054000
for ea,value in [(0x605525E,0x4021),(0x605527E,0x882E),(0x605532A,0x882B),(0x605531E,0xE028),(0x6055322,0x0ED4)]:
 assert int.from_bytes(data[ea-base:ea-base+2],'big')==value,f'Native actor frame contract changed at {ea:08X}'
print('PASS original native actor animation boundary operands')
