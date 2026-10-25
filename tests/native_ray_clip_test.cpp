#include "lagi/native_ray_clip.h"
#include <cassert>
#include <cstdio>
int main(){
 for(int denominator=2;denominator<2048;++denominator)for(int numerator=1;numerator<denominator;++numerator) {
  const unsigned expected=unsigned((std::int64_t(numerator)*256)/denominator)<<8;
  assert(lagi::nativeClipFraction(unsigned(numerator),unsigned(denominator))==expected);
  // The native sequence omits signed-division correction: an exactly
  // representable negative/negative quotient retains the previous bin.
  const unsigned signedExpected=unsigned((std::int64_t(numerator)*256-1)/denominator)<<8;
  assert(lagi::nativeClipFraction(unsigned(-numerator),unsigned(-denominator))==signedExpected);
 }
 assert(lagi::nativeClipFraction(unsigned(-1),unsigned(-2))==0x7f00);
 static constexpr int destinations[16][4]={
  {-1,-1,-1,-1},{1,-1,-1,-1},{-1,0,-1,-1},{3,2,-1,-1},
  {-1,-1,3,-1},{-1,-1,-1,-1},{-1,0,3,-1},{3,3,3,-1},
  {-1,-1,-1,2},{1,-1,-1,2},{-1,-1,-1,-1},{2,2,-1,2},
  {-1,-1,1,0},{1,-1,1,1},{-1,0,0,0},{-1,-1,-1,-1}};
 for(unsigned axis=0;axis<2;++axis)for(bool lower:{false,true})for(unsigned mask=0;mask<16;++mask) {
  lagi::NativeRayQuad q{};
  for(unsigned i=0;i<4;++i){q[i][axis]=(mask&(1u<<i))?(lower?-100:100):(lower?100:-100);q[i][axis^1]=int(i)*40;q[i][2]=1234+int(i);}
  const auto original=q;const auto result=lagi::nativeClipQuadPlane(q,axis,0,lower);
  const bool rejected=mask==5||mask==10||mask==15;
  assert(result==(rejected?2:mask?1:0));
  if(rejected){assert(q==original);continue;}
  for(unsigned i=0;i<4;++i){
   assert(q[i][2]==original[i][2]);
   if(mask&(1u<<i)) {
    assert(q[i][axis]==0);
    const int inside=destinations[mask][i];assert(inside>=0);
    const auto fraction=(lower?128:127)*256;
    const auto delta=std::int64_t(original[unsigned(inside)][axis^1]-original[i][axis^1]);
    assert(q[i][axis^1]==original[i][axis^1]+((delta*fraction)>>16));
   }
   else assert(q[i]==original[i]);
  }
 }
 // Native near-plane callback interpolates both X and Y, with DIV0U.
 // Exercise every fixed-quad case with nonuniform depths/neighbor choices.
 for(unsigned mask=0;mask<16;++mask)for(unsigned variant=0;variant<17;++variant){
  lagi::NativeRayQuad quad{};
  for(unsigned i=0;i<4;++i)quad[i]={int(i)*103-207,int(i)*-67+139,
      (mask&(1u<<i))?int(10+i*3):int(110+i*int(variant))};
  const auto before=quad;
  const unsigned result=lagi::nativeClipQuadPlane(quad,2,100,true);
  const bool reject=mask==5||mask==10||mask==15;
  assert(result==(reject?2:mask?1:0));
  if(reject){assert(quad==before);continue;}
  for(unsigned i=0;i<4;++i){
   if(!(mask&(1u<<i))){assert(quad[i]==before[i]);continue;}
   int in=destinations[mask][i];
   if((mask&(mask-1))==0){
    constexpr unsigned first[4]={1,0,3,2},second[4]={3,2,1,0};
    const unsigned a=first[i],b=second[i];
    in=int(before[a][2]>=before[b][2]?a:b);
   }
   const std::int64_t numerator=100-before[i][2],denominator=before[in][2]-before[i][2];
   const auto fraction=(numerator*256/denominator)*256;
   for(unsigned axis=0;axis<2;++axis)
    assert(quad[i][axis]==before[i][axis]+(((std::int64_t(before[in][axis])-before[i][axis])*fraction)>>16));
   assert(quad[i][2]==100);
  }
 }
 for(unsigned mask=0;mask<16;++mask){
  lagi::NativeRayQuad quad{};
  for(unsigned i=0;i<4;++i)quad[i]={0,0,(mask&(1u<<i))?300:100};
  assert(lagi::nativeClipPolygonDepth(quad,100,300)==(mask==0));
 }
 lagi::NativeRayQuad q={{{-20,20,0},{20,20,0},{20,-20,0},{-20,-20,0}}};
 assert(lagi::nativeClipRayQuad(q,-10,10,10,-10));
 for(const auto& p:q)assert(p[0]>=-10&&p[0]<=10&&p[1]>=-10&&p[1]<=10);
 q={{{-20,20,0},{-15,20,0},{-15,-20,0},{-20,-20,0}}};
 assert(!lagi::nativeClipRayQuad(q,-10,10,10,-10));
 for(bool reversed:{false,true}) {
  std::array<std::array<std::int32_t,3>,2> line={{{-100,200,0},{100,-200,200}}};
  if(reversed)std::swap(line[0],line[1]);
  assert(lagi::nativeClipRayDepth(line,100,300));
  const auto& clipped=line[reversed?1:0];
  assert(clipped[0]==0 && clipped[1]==0 && clipped[2]==100);
 }
 for(int a:{0,99,100,101,299,300,301})for(int b:{0,99,100,101,299,300,301}) {
  std::array<std::array<std::int32_t,3>,2> line={{{0,0,a},{0,0,b}}};
  const bool expected=(a>=100||b>=100) && a<300 && b<300;
  assert(lagi::nativeClipRayDepth(line,100,300)==expected);
  if(expected)for(const auto& p:line)assert(p[2]>=100 && p[2]<300);
 }
 puts("PASS native screen-ray clipper: eight-step signed fractions, all 16 masks/both axes/sides, unchanged/rejected ownership and four-plane sequence; renderer integration/device pending");
}
