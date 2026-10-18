#include <array>
#include <vector>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int32_t;using u32=uint32_t;using s16=int16_t;using u8=uint8_t;
struct fixedPoint{int m_value;fixedPoint(int n=0):m_value(n){}operator int()const{return m_value;}};
struct sVec3_FP{fixedPoint m0_X,m4_Y,m8_Z;};
struct s_workArea{virtual ~s_workArea()=default;char cppPrefix[103]{};};
struct s_DataTable2Sub0{sVec3_FP m4_position;std::array<s16,3> m10_rotation{};};
struct sParticleSlot{};
struct sVdp1Quad{};
struct Quads{std::vector<sVdp1Quad> m_waterfallMistQuadA,m_waterfallMistQuadB,m_waterfallFallingQuad;}quads;
auto* gFLD_A3=&quads;
struct sParticleSpawnConfig{
 sVec3_FP *m0_pPosition,*m4_pVelocity;const std::vector<sVdp1Quad>* m8_pQuadData;
 fixedPoint mC_velocityScaleX,m10_velocityScaleY;
 s32(*m14_updateFunc)(sParticleSlot*);s32 m18_heapSize;void* m1C_heapData;
};
struct Field{void* m168_particlePool=reinterpret_cast<void*>(1);}field;
using s_fieldSpecificData_A3=Field;
auto* getFieldSpecificData_A3(){return &field;}
s32 particleUpdateMoving(sParticleSlot*){return 0;}
unsigned rng=0;u32 randomNumber(){++rng;return 0;}
s32 centeredRandom(s32 mask){return s32(randomNumber()&mask)-(mask>>1);}
struct Emission{sVec3_FP p,v;const std::vector<sVdp1Quad>* q;int sx,sy;};std::vector<Emission> emissions;
int spawnParticleInPool(void*,sParticleSpawnConfig* c,int mode){
 assert(mode==1&&c->m14_updateFunc==particleUpdateMoving&&!c->m18_heapSize&&!c->m1C_heapData);
 emissions.push_back({*c->m0_pPosition,*c->m4_pVelocity,c->m8_pQuadData,int(c->mC_velocityScaleX),int(c->m10_velocityScaleY)});return 1;
}
#include "waterfall_helpers.inc"
struct Small:s_workArea{char extra[31];s_DataTable2Sub0* m8;u8 mC_frameCounter=3;};
struct Large:s_workArea{char extra[59];s_DataTable2Sub0* m8;u8 m10_frameCounter=8;};
int main(){
 s_DataTable2Sub0 data;data.m4_position={0x100000,0x200000,0x300000};
 Small small;small.m8=&data;Large large;large.m8=&data;
 waterfallSpawnRisingMist(&small);
 assert(int(emissions.back().p.m0_X)==0x100000-(0x3FFFF>>1)&&emissions.back().q==&quads.m_waterfallMistQuadA);
 waterfallSpawnSplashMist(&small);
 assert(int(emissions.back().p.m8_Z)==0x310000&&emissions.back().q==&quads.m_waterfallMistQuadB);
 waterfallSpawnRisingMistVariant(&small);
 assert(int(emissions.back().p.m8_Z)==0x308000);
 waterfallSpawnDroplet(&small);
 assert(int(emissions.back().p.m0_X)==0x100000-0x4000-(0x7FFF>>1));
 assert(int(emissions.back().p.m8_Z)==0x316000&&emissions.back().q==&quads.m_waterfallFallingQuad);
 waterfallLargeSpawnFalling(&large);
 assert(int(emissions.back().p.m0_X)==0x100000-0xD000);
 waterfallLargeSpawnSplash(&large,0x40000);
 assert(int(emissions.back().p.m0_X)==0x100000-(0x1FFFF>>1));
 assert(emissions.size()==6&&rng==26);
 field.m168_particlePool=nullptr;waterfallSpawnRisingMist(&small);assert(emissions.size()==6);
 std::puts("Actual waterfall helpers use typed source/counter across shifted small/large task layouts PASS");
}
