#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
using u16=std::uint16_t;
using sVec3_FP=std::array<int,3>;
struct quadColor {u16 c[4];};
struct Allocation {u16 m4_vdp1Memory=0x3100;} allocation;
struct sSmokePuffTask {Allocation* m4_vd1Allocation=&allocation;sVec3_FP m28_position{1,2,3},m4C_targetPos{4,5,6};};
int draws=0;
void displayRaySegment(std::array<sVec3_FP,2>& points,int width,u16 address,int size,u16 color,const quadColor* gouraud,int mode){
 ++draws;assert(points[0]==sVec3_FP({1,2,3})&&points[1]==sVec3_FP({4,5,6}));
 assert(width==0x400&&address==u16(0x3100+0xCD0)&&size==0x510&&color==u16(0x3100+0x2EEC)&&mode==0x108);
 for(auto c:gouraud->c)assert(c==0x4210);
}
#include "production.inc"
int main(){sSmokePuffTask task;smokePuff_draw(&task);assert(draws==1);puts("PASS production Arachnoth strand endpoints, texture/palette relocation, width, size, mesh stipple and neutral colors; not device acceptance");}
