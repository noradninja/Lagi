#include "lagi/debug_mesh.h"
#include "lagi/lagi_live_model_adapter.h"
#include "lagi/lagi_render_bridge.h"
#include "lagi/lagi_projection_offset.h"
#include "lagi/vdp1_color_calculation.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace azel=lagi::azel;
namespace azel_bridge=lagi::azel_bridge;
#include "append_types.inc"
static azel::BasicWingDebugMesh g_liveTownCpuMesh;
static bool g_profileAppendSample=false;
static std::uint64_t g_profileAppendSetupUs=0,g_profileAppendStorageUs=0;
static std::uint64_t g_profileAppendTransformUs=0,g_profileAppendLightUs=0,g_profileAppendBindingUs=0;
static std::uint64_t sceKernelGetProcessTimeWide(){static std::uint64_t clock=0;return ++clock;}
static std::vector<LivePolygonLightState> g_liveTownPolygonLights;
struct LiveTownMeshRange {std::size_t first,count;};
static std::vector<LiveTownMeshRange> g_liveTownMeshRanges;
static float g_townCameraTarget[3]={1,2,8},g_townCameraPosition[3]={0,0,0},g_townCameraUp[3]={0,1,0};
static bool g_nativeSceneViewValid=true;
static float g_nativeSceneViewMatrix[12]={1,0,0,0,0,1,0,0,0,0,1,0};
static float g_azelProjectionFovDegrees=60;
constexpr int kWidth=960,kHeight=544;
void azelProjectionScales(float,unsigned,float& x,float& y){x=1.2f;y=1.4f;}
std::uint16_t liveTownTextureIndex(const azel::SaturnPolygonRecord& r){return (r.cmdSrca+r.cmdColr)&0xffff;}
namespace logging {void writef(const char*,...) {}}
#include "append_body.inc"
struct Output {
 azel::BasicWingDebugMesh mesh;std::vector<LivePolygonLightState> lights;
 std::vector<LiveTownMeshRange> ranges;
};
void reset(){g_liveTownCpuMesh={};g_liveTownPolygonLights.clear();g_liveTownMeshRanges.clear();}
Output capture(){return {g_liveTownCpuMesh,g_liveTownPolygonLights,g_liveTownMeshRanges};}
void same(const Output& a,const Output& b){
 assert(a.mesh.vertices.size()==b.mesh.vertices.size());
 assert(a.mesh.lightingVertices.size()==b.mesh.lightingVertices.size());
 for(unsigned i=0;i<a.mesh.vertices.size();++i){
  assert(!std::memcmp(&a.mesh.vertices[i],&b.mesh.vertices[i],sizeof(azel::DebugColorVertex)));
  assert(!std::memcmp(&a.mesh.lightingVertices[i],&b.mesh.lightingVertices[i],sizeof(azel::DebugColorVertex)));
 }
 assert(a.mesh.polygonTextureIndices==b.mesh.polygonTextureIndices);
 assert(a.mesh.polygonRecords.size()==b.mesh.polygonRecords.size());
 for(unsigned i=0;i<a.mesh.polygonRecords.size();++i){
  const auto& x=a.mesh.polygonRecords[i];const auto& y=b.mesh.polygonRecords[i];
  assert(x.model==y.model&&x.polygonInModel==y.polygonInModel&&x.lightingControl==y.lightingControl&&x.lightingCount==y.lightingCount);
  assert(x.cmdCtrl==y.cmdCtrl&&x.cmdPmod==y.cmdPmod&&x.cmdSrca==y.cmdSrca&&x.cmdSize==y.cmdSize&&x.cmdColr==y.cmdColr);
  for(unsigned c=0;c<4;++c){assert(x.indices[c]==y.indices[c]);
   assert(x.lighting[c].hasColor==y.lighting[c].hasColor);
   for(unsigned k=0;k<3;++k){assert(x.lighting[c].normal[k]==y.lighting[c].normal[k]);assert(x.lighting[c].color[k]==y.lighting[c].color[k]);}
  }
  assert(!std::memcmp(&a.mesh.gouraud555[i],&b.mesh.gouraud555[i],sizeof(azel::SaturnGouraud555Quad)));
 }
 assert(a.lights.size()==b.lights.size());
 for(unsigned i=0;i<a.lights.size();++i){const auto& x=a.lights[i];const auto& y=b.lights[i];
  assert(x.valid==y.valid&&x.hasNativeViewDepth==y.hasNativeViewDepth&&x.nativeViewDepthRaw==y.nativeViewDepthRaw);
  assert(x.additionalLight.active==y.additionalLight.active);
  for(unsigned k=0;k<3;++k){assert(x.vector[k]==y.vector[k]&&x.color[k]==y.color[k]&&x.falloff[k]==y.falloff[k]);
   assert(x.additionalLight.vector[k]==y.additionalLight.vector[k]&&x.additionalLight.color[k]==y.additionalLight.color[k]);}
 }
 assert(a.ranges.size()==b.ranges.size());for(unsigned i=0;i<a.ranges.size();++i)assert(a.ranges[i].first==b.ranges[i].first&&a.ranges[i].count==b.ranges[i].count);
}
int main(){
 azel_bridge::LiveVdp1Model model;
 model.polygons.resize(7);model.vertices.resize(42);
 const unsigned modes[]={0,0x100,0x100,3,0,0x100,7};
 for(unsigned p=0;p<7;++p){auto& r=model.polygons[p];r.cmdPmod=modes[p];r.cmdSrca=p*17;r.cmdColr=p*3;r.cmdSize=0x210;r.lightingControl=0x200;r.lightingCount=4;
  for(unsigned c=0;c<4;++c){r.indices[c]=c;r.lighting[c].normal[0]=p*9+c;r.lighting[c].color[1]=p*11+c;r.lighting[c].hasColor=true;}
  for(unsigned c=0;c<6;++c)model.vertices[p*6+c]={float(c),-float(c),float(c*2),std::uint8_t(17+p),std::uint8_t(29+p),41,255};
 }
 std::uint16_t resolved[7]={7,6,5,4,3,2,1};
 for(unsigned variant=0;variant<32;++variant){
  model.positionSources.clear();
  if(variant&1)for(unsigned p=0;p<7;++p)for(unsigned c=0;c<6;++c)model.positionSources.push_back(c);
  g_profileAppendSample=(variant&16)!=0;
  azel_bridge::SubmissionState state{};state.hasModelMatrix=variant!=0;state.hasLight=variant&1;state.billboard=variant&2;state.hasNativeModelLight=variant&4;
  state.localCoordinatesX=variant&8?190:176;state.localCoordinatesY=variant&8?99:112;
  state.modelMatrix[0]=state.modelMatrix[5]=state.modelMatrix[10]=65536;state.modelMatrix[3]=65536;state.modelMatrix[7]=-131072;state.modelMatrix[11]=196608;
  state.lightVector[0]=100;state.lightVector[1]=-200;state.lightVector[2]=300;
  state.nativeModelLight[0]=40;state.nativeModelLight[1]=-50;state.nativeModelLight[2]=60;
  state.lightColor[0]=20;state.lightColor[1]=30;state.lightColor[2]=40;state.lightFalloff[0]=12;state.lightFalloff[1]=23;state.lightFalloff[2]=34;
  state.nativeViewDepthRaw=0x12345;state.hasNativeViewDepth=true;state.additionalLight.active=true;state.additionalLight.vector[1]=4;state.additionalLight.color[2]=19;
  for(bool generic: {false,true}){
   const auto last=model.vertices.back();
   if(generic)model.vertices.pop_back();
   reset();appendLiveTownModelReference(model,state,resolved,variant&16?3:7);for(unsigned repeat=0;repeat<64;++repeat)appendLiveTownModelReference(model,state);auto expected=capture();
   reset();appendLiveTownModel(model,state,resolved,variant&16?3:7);for(unsigned repeat=0;repeat<64;++repeat)appendLiveTownModel(model,state);same(expected,capture());
   if(generic)model.vertices.push_back(last);
  }
 }
 puts("PASS production model append versus accepted baseline: geometry, materials, lighting, blend/mesh order, repeated appends and fallback layout; not Vita timing");
}
