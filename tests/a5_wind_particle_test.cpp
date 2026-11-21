#include <array>
#include <vector>
#include <map>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iterator>
using u8=uint8_t;using s8=int8_t;using u16=uint16_t;using s16=int16_t;using u32=uint32_t;using s32=int32_t;using s64=int64_t;
#include "fixedPoint.h"
struct sVec3_FP {fixedPoint m0_X,m4_Y,m8_Z;};
using quadColor=std::array<u16,4>;
struct sSaturnPtr;
#include "kernel/vdp1AnimatedQuad.h"
struct sSaturnPtr {u32 m_offset;sSaturnPtr& operator+=(u32 n){m_offset+=n;return *this;} sSaturnPtr operator+(u32 n)const{return {m_offset+n};}};
static std::vector<u8> prg;
u32 read(sSaturnPtr p,unsigned count) {
 const auto o=p.m_offset-0x6054000;assert(o+count<=prg.size());u32 v=0;
 for(unsigned i=0;i<count;++i)v=(v<<8)|prg[o+i];return v;
}
u8 readSaturnU8(sSaturnPtr p){return u8(read(p,1));}
u16 readSaturnU16(sSaturnPtr p){return u16(read(p,2));}
u32 readSaturnU32(sSaturnPtr p){return read(p,4);}
fixedPoint readSaturnFP(sSaturnPtr p){return s32(read(p,4));}
struct Overlay {sSaturnPtr getSaturnPtr(u32 ea){return {ea};}};
static Overlay overlay;static Overlay* gFLD_A5=&overlay;
struct Particle {
 sAnimatedQuad m0_quad;sVec3_FP m8_position;
 s32 m14_velocityX=0,m18_velocityY=0,m1C_velocityZ=0,m24_gravityY=0;
 s32 m2C_orbitAngle=0,m30_orbitAngleSpeed=0,m34_pad2=0,m38_orbitRadius=0,m3C_orbitRadiusSpeed=0;
 s8 m40_lifetime=0;u8 m41_type=0;
};using sA5SandParticle=Particle;
struct sA5SandParticlePool {
 std::array<s16,196> m8_freeIndices{},m190_sortedIndices{};
 std::array<Particle,196> m318_particles{};int m3728_drawCount=0;
};
static sA5SandParticlePool pool;
sA5SandParticlePool* getSandParticlePool(){return &pool;}
struct sA5ExitEntity {int m18=0,m34=0,m1C_exitTargetX_mode1=0,m20_exitTargetZ_mode1=0,m24_exitTargetX_mode3=0;};
struct FieldData {sA5ExitEntity* m4_pExitEntity;};using s_fieldSpecificData_A5=FieldData;
static sA5ExitEntity exitEntity;static FieldData data{&exitEntity};
struct Field {FieldData* mC;};static Field field{&data};Field* getFieldTaskPtr(){return &field;}
struct {sVec3_FP m0_position;}cameraProperties2;
static unsigned randomIndex=0,randomCalls=0;
u32 randomNumber(){return randomCalls++==0?randomIndex:0;}
void a5ExitEntity_WindStateMachine(sA5ExitEntity*){}
s32 performModulo2(s32 divisor,s32 dividend){return dividend%divisor;}
fixedPoint getSin(unsigned){return 0;}fixedPoint getCos(unsigned){return 0x10000;}
fixedPoint MTH_Mul(fixedPoint a,fixedPoint b){return s32((int64_t(a.asS32())*b.asS32())>>16);}
// Rebinding must preserve the original per-slot character base and phase.
void particleInitSub(sAnimatedQuad*,u16,const std::vector<sVdp1Quad>*){assert(false);}
// PRODUCTION_WIND
void resetPool() {
 pool={};for(int i=0;i<196;++i)pool.m8_freeIndices[i]=s16(195-i);
 for(auto& p:pool.m318_particles){p.m0_quad.m4_vdp1Memory=0x2345;p.m0_quad.m6=3;p.m0_quad.m7_currentFrame=4;}
}
void checkPhase(const Particle& p){assert(p.m0_quad.m4_vdp1Memory==0x2345&&p.m0_quad.m6==3&&p.m0_quad.m7_currentFrame==4);}
int main(int argc,char** argv) {
 assert(argc==2);std::ifstream in(argv[1],std::ios::binary);prg.assign(std::istreambuf_iterator<char>(in),{});assert(!prg.empty());
 const u32 addresses[]={0x60992B4,0x6099394,0x6099394,0x6099474};
 for(unsigned index=0;index<4;++index) {
  auto* quads=lagiA5WindQuadList(0x6099804,index);
  assert(quads==a5GetOrParseQuadList({addresses[index]})&&quads->size()==8);
  for(auto& q:*quads)assert(q.mC_width>0&&q.mC_width<=0x20000&&q.m10_height>0&&q.m10_height<=0x20000);
  resetPool();exitEntity.m18=0x2800;exitEntity.m34=0;randomIndex=index;randomCalls=0;
  a5ExitEntity_Mode0(&exitEntity);assert(pool.m3728_drawCount==1);
  auto& p=pool.m318_particles[195];checkPhase(p);
  assert(p.m0_quad.m0_quad==quads&&p.m40_lifetime==10&&p.m41_type==0&&p.m24_gravityY==0x51);
  resetPool();exitEntity.m34=8;randomIndex=index;randomCalls=0;
  a5ExitEntity_Mode3(&exitEntity);auto& falling=pool.m318_particles[195];checkPhase(falling);
  assert(falling.m0_quad.m0_quad==quads&&falling.m40_lifetime==0x50&&falling.m41_type==2);
 }
 auto* quads=lagiA5WindQuadList(0x6099814,0);assert(quads==a5GetOrParseQuadList({0x6099554}));
 assert(lagiA5WindQuadList(0x6099814,1)==a5GetOrParseQuadList({0x6099634}));
 resetPool();a5ExitEntity_spawnDirectionalParticle(100,0,8,quads);
 auto& p=pool.m318_particles[195];checkPhase(p);assert(p.m0_quad.m0_quad==quads&&p.m40_lifetime==8&&p.m41_type==1);
 pool.m3728_drawCount=196;sVec3_FP position;
 a5ExitEntity_spawnWormParticle(&position,10,quads);assert(pool.m3728_drawCount==196);
 // The old mode-0 "list" begins with a Saturn pointer, not a valid frame.
 assert(readSaturnU8({0x6099804})==6);
 assert(readSaturnU32({0x6099804+12})==0x6099474); // would be a huge sprite width
 std::puts("PASS production wind table resolution and emitters: all modes, native lists, preserved VRAM/phase, ground sand gravity and full-pool rejection");
}
