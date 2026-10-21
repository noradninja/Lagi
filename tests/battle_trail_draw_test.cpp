#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using s32=int;using u16=std::uint16_t;using sVec3_FP=std::array<int,3>;using quadColor=std::array<u16,4>;
struct sSaturnPtr {int value=0;sSaturnPtr operator+(int n)const{return {value+n};}};
std::array<unsigned char,512> memory{};
int readSaturnS32(sSaturnPtr p){std::uint32_t n=0;for(int i=0;i<4;++i)n=(n<<8)|memory[p.value+i];return static_cast<int>(n);}
u16 readSaturnU16(sSaturnPtr p){return (memory[p.value]<<8)|memory[p.value+1];}
short readSaturnS16(sSaturnPtr p){return static_cast<short>(readSaturnU16(p));}
sSaturnPtr readSaturnEA(sSaturnPtr p){return {readSaturnS32(p)};}
void put(int at,unsigned n,int bytes){while(bytes){memory[at+--bytes]=n&255;n>>=8;}}
struct sTrailRenderer {sVec3_FP* m0_positions;int m4_numPositions,m8_visibleCount; sSaturnPtr m10_renderData;u16 m14_vdp1Base;};
int calls=0;bool expectedGouraud=false;
void displayRaySegment(std::array<sVec3_FP,2>& p,int width,u16 address,short size,u16 palette,const quadColor* c,int mode){
 const int segment=2-calls;
 assert(p[0][0]==segment+1&&p[1][0]==segment);
 assert(width==int(100+segment*73)&&address==0x3123&&size==0x510&&palette==0x3456&&mode==8);
 for(int i=0;i<4;++i)assert((*c)[i]==(expectedGouraud?0x4210+segment*8+i:0x4210));
 ++calls;
}
#include "../src/integration/lagi_battle_trail_draw.inc"
int main(){
 put(4,0x123,2);put(6,0x510,2);put(8,0x456,2);put(12,64,4);put(16,128,4);
 for(int i=0;i<3;++i){put(64+i*4,100+i*73,4);for(int c=0;c<4;++c)put(128+i*8+c*2,0x4210+i*8+c,2);}
 sVec3_FP points[4]={{{0,0,0}},{{1,0,0}},{{2,0,0}},{{3,0,0}}};
 sTrailRenderer trail{points,4,0,{0},0x3000};
 trailRenderer_draw(&trail);assert(calls==3);
 calls=0;expectedGouraud=true;lagiDrawBattleTrailGouraud(&trail);assert(calls==3);
 calls=0;trail.m8_visibleCount=2;lagiDrawBattleTrailGouraud(&trail);assert(calls==1);
 calls=0;trail.m0_positions=nullptr;trailRenderer_draw(&trail);assert(calls==0);
 puts("PASS production trail draw: descriptor widths, per-segment colors, relocated texture/palette, segment order and visibility");
}
