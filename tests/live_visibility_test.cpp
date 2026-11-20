#include <vector>
#include <cstdint>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <cstdio>
namespace azel {struct DebugColorVertex {float x,y,z;};}
struct ViewerMat4 {float m[16]{};};
static struct {std::vector<int> polygonRecords;std::vector<azel::DebugColorVertex> vertices;} g_liveTownCpuMesh;
static unsigned g_profileGouraudPrepUs=0,g_profileGouraudVisibleQuads=0,g_profileGouraudTotalQuads=0;
static std::uint64_t sceKernelGetProcessTimeWide(){static std::uint64_t t=0;return ++t;}
static bool halfResolution=false;
static int viewerRenderWidth(){return halfResolution?480:960;}
static int viewerRenderHeight(){return halfResolution?272:544;}
#include "visibility.inc"
static std::uint32_t seed=12345;
static float randomFloat(){seed=seed*1664525u+1013904223u;return float(int(seed>>8)%8193-4096)/1024.0f;}
int main(){
 unsigned visible=0,hidden=0;
 for(bool half:{false,true})for(unsigned sample=0;sample<512;++sample){
  halfResolution=half;
  g_liveTownCpuMesh.polygonRecords.resize(sample%33);
  g_liveTownCpuMesh.vertices.resize((sample%33)*6);
  for(auto& v:g_liveTownCpuMesh.vertices)v={randomFloat(),randomFloat(),randomFloat()};
  ViewerMat4 wvp;
  for(auto& f:wvp.m)f=randomFloat();
  if(sample%4==0){wvp={};wvp.m[0]=wvp.m[5]=wvp.m[10]=wvp.m[15]=1;}
  if(sample%8==0)wvp.m[15]=0.00001f;
  if(sample%16==0)wvp.m[15]=-1;
  prepareReference(wvp);
  const auto expected=g_liveTownGouraudPrep;
  const auto expectedCount=g_profileGouraudVisibleQuads;
  prepareLiveTownGouraudVisibility(wvp);
  assert(g_profileGouraudVisibleQuads==expectedCount);
  assert(g_liveTownGouraudPrepVertices==g_liveTownCpuMesh.vertices.data());
  for(unsigned p=0;p<expected.size();++p){
   auto& actual=g_liveTownGouraudPrep[p];
   assert(actual.visible==expected[p].visible);
   for(const auto& corner:actual.screen)assert(!corner.valid); // Accelerated path does no projection.
   projectLiveTownGouraudScreen(wvp,p,actual);
   if(actual.visible){
    ++visible;
    for(unsigned c=0;c<4;++c){
     assert(actual.screen[c].valid==expected[p].screen[c].valid);
     assert(!std::memcmp(&actual.screen[c].x,&expected[p].screen[c].x,sizeof(float)));
     assert(!std::memcmp(&actual.screen[c].y,&expected[p].screen[c].y,sizeof(float)));
    }
    const auto saved=actual;
    projectLiveTownGouraudScreen(wvp,p,actual);
    assert(!std::memcmp(&saved,&actual,sizeof(actual))); // Second fallback request reuses payload.
   }else {++hidden;for(const auto& corner:actual.screen)assert(!corner.valid);}
  }
 }
 assert(visible && hidden);
 g_liveTownCpuMesh.vertices.resize(1);
 prepareLiveTownGouraudVisibility({});assert(!g_liveTownGouraudPrepValid);
 std::printf("PASS production lazy visibility: %u visible/%u hidden quads, exact baseline visibility and fallback XY bytes, full/half resolution, near/behind clip cases and invalid layout\n",visible,hidden);
}
