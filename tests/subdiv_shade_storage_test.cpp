#include <array>
#include <vector>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <cstdio>
// PRODUCTION_STATE
// PRODUCTION_GRID
struct Vertex {float x=0,y=0,z=0,u=0,v=0,shadeR=0,shadeG=0,shadeB=0;};
struct Shade {float corner[4][3]{};};
static std::vector<Vertex> payload;
static Vertex* g_vdp1SubdivVertices=nullptr;
static bool g_fieldProbeActive=false;
static unsigned g_profileSubdivShadeWrittenQuads=0,g_profileSubdivShadeReusedQuads=0;
void cached(unsigned p,const Shade& shade) {
 const bool updatePosition=false;float gridX[9]{},gridY[9]{},gridZ[9]{};
 const unsigned baseVertex=p*9;
// PRODUCTION_CALCULATE
// PRODUCTION_WRITE
}
std::array<float,27> reference(const Shade& shade) {
 std::array<float,27> out{};
 for(int channel=0;channel<3;++channel) {
  float values[9];fillMidpointGrid3x3(shade.corner[0][channel],shade.corner[1][channel],shade.corner[2][channel],shade.corner[3][channel],values);
  for(int i=0;i<9;++i)out[i*3+channel]=values[i];
 }
 return out;
}
void check(unsigned p,const Shade& shade) {
 const auto expect=reference(shade);
 for(unsigned i=0;i<9;++i) {
  const auto& v=payload[p*9+i];float actual[]={v.shadeR,v.shadeG,v.shadeB};
  assert(std::memcmp(actual,expect.data()+i*3,sizeof(actual))==0);
 }
}
uint32_t rng=0xA521;uint32_t next(){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main() {
 constexpr unsigned count=128;
 payload.resize(count*9);g_vdp1SubdivVertices=payload.data();
 std::array<Shade,count> colors{};
 for(auto& c:colors)for(auto& corner:c.corner)for(float& f:corner)f=(int(next()&31)-16)/32.f;
 prepareSubdivShadeStorage(false);g_sceneSubdivShadeStates.resize(count);
 for(unsigned p=0;p<count;++p){cached(p,colors[p]);check(p,colors[p]);}
 assert(g_profileSubdivShadeWrittenQuads==count);
 for(unsigned frame=0;frame<512;++frame) {
  // Geometry/material identity changes at every slot, but shade payload has
  // no dependency on identity, XYZ, UV, texture or camera.
  prepareSubdivShadeStorage(true);g_sceneSubdivShadeStates.resize(count);
  for(auto& v:payload){v.x=float(next());v.y=float(frame);v.u=float(next()&255);}
  // New shade values at a subset of slots must still invalidate exact bits.
  if(frame%7==0)colors[frame%count].corner[frame%4][frame%3]+=1.f/32.f;
  g_fieldProbeActive=(frame%5)==0;
  for(unsigned p=0;p<count;++p) {
   if((p+frame)%9==0)continue; // reveal previously skipped slots later
   cached(p,colors[p]);check(p,colors[p]);
  }
 }
 assert(g_profileSubdivShadeReusedQuads>50000);
 // Equal float values with different bit patterns still invalidate the key.
 colors[0].corner[0][0]=0.0f;cached(0,colors[0]);
 const auto signedZeroBefore=g_profileSubdivShadeWrittenQuads;
 colors[0].corner[0][0]=-0.0f;cached(0,colors[0]);check(0,colors[0]);
 assert(g_profileSubdivShadeWrittenQuads==signedZeroBefore+1);
 // A new mapped stream must never reuse keys from the previous storage.
 const auto before=g_profileSubdivShadeWrittenQuads;
 std::fill(payload.begin(),payload.end(),Vertex{});prepareSubdivShadeStorage(false);
 g_sceneSubdivShadeStates.resize(count);
 for(unsigned p=0;p<count;++p){cached(p,colors[p]);check(p,colors[p]);}
 assert(g_profileSubdivShadeWrittenQuads==before+count);
 // Shrinking/growing slot count discards removed keys and initializes new ones.
 g_sceneSubdivShadeStates.resize(8);g_sceneSubdivShadeStates.resize(count);
 for(unsigned p=8;p<count;++p)assert(!g_sceneSubdivShadeStates[p].valid);
 std::printf("PASS production shade cache: bit-exact payloads across 512 geometry/material rebinds, changed corners, visibility/probe changes, storage replacement and slot shrink/growth; reused=%u written=%u\n",g_profileSubdivShadeReusedQuads,g_profileSubdivShadeWrittenQuads);
}
