#include <vector>
#include <map>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u32=std::uint32_t;using u16=std::uint16_t;using u8=std::uint8_t;
struct sSaturnMemoryFile{}fileA,fileB;
struct sSaturnPtr{sSaturnMemoryFile* m_file;u32 m_offset;};
struct sVdp1Quad{u8 m0_isLast,m1;u16 m4_CMDPMOD;};
int parses=0;
std::vector<sVdp1Quad> initVdp1Quad(sSaturnPtr source){
 ++parses;assert(source.m_offset==0x060A9814||source.m_offset==0x060A999C);
 const bool second=source.m_offset==0x060A999C;
 std::vector<sVdp1Quad> result(14);
 for(unsigned i=0;i<14;++i)result[i]={u8(i==13),u8(second||i==13?4:2),u16(second?0x188:0x88)};
 return result;
}
struct sAnimatedQuad{const std::vector<sVdp1Quad>* m0_quad;u16 m4_vdp1Memory=0;u8 m6=0,m7_currentFrame=0;};
#include "../src/integration/lagi_battle_particle_frames.inc"
#include "step.inc"
int main(){
 const sSaturnPtr a{&fileA,0x060A9814},b{&fileA,0x060A999C};
 sAnimatedQuad smoke{&lagiBattleParticleFrames(a)};
 for(int i=0;i<20;++i)assert(!(sGunShotTask_UpdateSub4(&smoke)&2));
 const auto* original=smoke.m0_quad;
 sAnimatedQuad rock{&lagiBattleParticleFrames(b)};
 assert(rock.m0_quad!=original&&original->at(0).m4_CMDPMOD==0x88&&rock.m0_quad->at(0).m4_CMDPMOD==0x188);
 assert(&lagiBattleParticleFrames(a)==original&&parses==2);
 int smokeEnd=0,rockEnd=0;
 for(int tick=1;tick<=70;++tick){
  if(!smokeEnd&&(sGunShotTask_UpdateSub4(&smoke)&2))smokeEnd=20+tick;
  if(!rockEnd&&(sGunShotTask_UpdateSub4(&rock)&2))rockEnd=tick;
 }
 assert(smokeEnd==44&&rockEnd==70);
 assert(&lagiBattleParticleFrames({&fileB,a.m_offset})!=original&&parses==3);
 assert(original->size()==14&&original->at(0).m1==2);
 puts("PASS simultaneous immutable particle animations: source/file isolation, native frame stepping, independent 44/70-tick sequences and stable borrowed storage");
}
