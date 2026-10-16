// Production GPU lifecycle with a mock driver: not a GPU execution/FPS test.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
using SceUID=int;
namespace azel {
struct DebugTextureVertex{float x,y,z,u,v;};
struct DebugColorVertex{float x,y,z;unsigned char r,g,b,a;};
}
struct SceGxmTexture{void* pixels=nullptr;};
struct SceDisplayFrameBuf{unsigned size;void* base;unsigned pitch,pixelformat,width,height;};
enum{SCE_GXM_MEMORY_ATTRIB_READ,SCE_GXM_TEXTURE_FORMAT_A8B8G8R8,SCE_GXM_TEXTURE_FILTER_POINT,
 SCE_GXM_TEXTURE_ADDR_CLAMP,SCE_GXM_CULL_NONE,SCE_GXM_POLYGON_MODE_TRIANGLE_FILL,
 SCE_GXM_DEPTH_FUNC_ALWAYS,SCE_GXM_DEPTH_WRITE_DISABLED,SCE_GXM_PRIMITIVE_TRIANGLES,
 SCE_GXM_INDEX_FORMAT_U16,SCE_DISPLAY_PIXELFORMAT_A8B8G8R8,SCE_DISPLAY_SETBUF_NEXTFRAME};
static constexpr int kWidth=32,kHeight=16,kPitch=32,kFrameBytes=kWidth*kHeight*4;
static constexpr int kGxmPitch=64;
static std::array<std::uint32_t,kWidth*kHeight> cpu[2];
static std::uint32_t* g_frameBuffer[2]={cpu[0].data(),cpu[1].data()};
static int g_drawBuffer=0,g_gxmDrawBuffer=1;
static int context=1,program=1,resource=1;
static auto* g_probeContext=&context;
static auto* g_textureVertexProgram=&program;
static auto* g_textureFragmentProgram=&program;
static auto* g_textureWvpParam=&program;
static auto* g_fadeFragmentProgram=&program;
static auto* g_probeVertexProgram=&program;
static auto* g_probeWvpParam=&program;
static auto* g_probeRenderTarget=&resource;
static auto* g_probeSync=&resource;
static auto* g_probeSync2=&resource;
static int g_probeColorSurface=0,g_probeColorSurface2=1;
static auto* g_probeColorBuffer=cpu[0].data();
static auto* g_probeColorBuffer2=cpu[1].data();
static azel::DebugColorVertex fadeVertices[6]{};
static std::uint16_t fadeIndices[6]{};
static auto* g_fadeVertices=fadeVertices;
static auto* g_fadeIndices=fadeIndices;
static bool g_viewerReady=true,g_debugVisible=true,g_probeDisplayingGxm=false,g_launcherPresentedGxm=false;
static unsigned allocations=0,compositions=0,presents=0,waits=0,draws=0,finishes=0;
static int failAllocation=-1,beginResult=0,uniformResult=0,drawResult=0,displayResult=0;
static std::map<int,void*> mapped;
static bool scene=false;
static void* probeGpuAlloc(unsigned bytes,int,SceUID* uid){
 if(int(allocations++)==failAllocation)return nullptr;
 *uid=int(allocations);return mapped[*uid]=std::malloc(bytes);
}
static int sceGxmUnmapMemory(void*){assert(!scene);return 0;}
static int sceKernelFreeMemBlock(int uid){assert(mapped.count(uid));std::free(mapped[uid]);mapped.erase(uid);return 0;}
static int sceGxmFinish(int*){assert(!scene);++finishes;return 0;}
static std::uint64_t sceKernelGetProcessTimeWide(){static std::uint64_t t=0;return ++t;}
static void show_game_presentation(){g_probeDisplayingGxm=g_viewerReady;g_debugVisible=!g_viewerReady;}
namespace logging{template<class... T>void writef(const char*,T...) {}}
static void launcherTextured(const bool available[4],int selected){
 ++compositions;unsigned mask=0;for(unsigned i=0;i<4;++i)if(available[i])mask|=1u<<i;
 for(unsigned i=0;i<cpu[0].size();++i)g_frameBuffer[g_drawBuffer][i]=0xFF000000u|mask<<16|(selected+1)<<8|i;
}
static void fill(std::uint32_t color){cpu[g_drawBuffer].fill(color);}
static int sceGxmTextureInitLinear(SceGxmTexture* texture,void* data,int,int,int,int){texture->pixels=data;return 0;}
template<class... T>void sceGxmTextureSetMinFilter(T...){}
template<class... T>void sceGxmTextureSetMagFilter(T...){}
template<class... T>void sceGxmTextureSetUAddrMode(T...){}
template<class... T>void sceGxmTextureSetVAddrMode(T...){}
template<class... T>int sceGxmBeginScene(T...){assert(!scene);scene=beginResult>=0;return beginResult;}
template<class... T>void sceGxmSetDefaultRegionClipAndViewport(T...){}
template<class... T>void sceGxmSetCullMode(T...){}
template<class... T>void sceGxmSetFrontPolygonMode(T...){}
template<class... T>void sceGxmSetBackPolygonMode(T...){}
template<class... T>void sceGxmSetFrontDepthFunc(T...){}
template<class... T>void sceGxmSetBackDepthFunc(T...){}
template<class... T>void sceGxmSetFrontDepthWriteEnable(T...){}
template<class... T>void sceGxmSetBackDepthWriteEnable(T...){}
template<class... T>void sceGxmSetVertexProgram(T...){}
template<class... T>void sceGxmSetFragmentProgram(T...){}
static int sceGxmReserveVertexDefaultUniformBuffer(int*,void** out){*out=&resource;return uniformResult;}
template<class... T>void sceGxmSetUniformDataF(T...){}
template<class... T>void sceGxmSetFragmentTexture(T...){}
template<class... T>void sceGxmSetVertexStream(T...){}
template<class... T>int sceGxmDraw(T...){++draws;return drawResult;}
template<class... T>int sceGxmEndScene(T...){assert(scene);scene=false;return 0;}
static int sceDisplaySetFrameBuf(SceDisplayFrameBuf* fb,int){assert(!scene);assert(fb->pitch==kGxmPitch);++presents;return displayResult;}
static int sceDisplayWaitVblankStart(){++waits;return 0;}
struct ViewerMat4{float m[16]{};};
static ViewerMat4 viewerIdentity(){return {};}
#include "gpu.inc"
#include "fade.inc"
#include "end_frame.inc"
int main(){
 const bool all[4]={true,true,true,true},partial[4]={true,false,true,false},none[4]={};
 assert(launcherPrepareGpu(all) && compositions==4 && mapped.size()==6);
 for(int selection=0;selection<4;++selection){
  launcherTextured(all,selection);
  assert(!std::memcmp(g_launcherGpuStates[selection+1].pixels,cpu[0].data(),kFrameBytes));
 }
 const auto before=compositions,allocated=allocations;
 for(unsigned frame=0;frame<64;++frame){assert(launcherPrepareGpu(all));assert(launcherDrawGpu(frame%4,255));end_frame();}
 assert(compositions==before && allocations==allocated && presents==64 && waits==64 && draws==64);
 assert(!launcherDrawGpu(-1,255));
 assert(launcherPrepareGpu(partial) && mapped.size()==4 && !g_launcherGpuStates[2].pixels);
 assert(launcherPrepareGpu(none) && mapped.size()==3 && launcherDrawGpu(-1,255));
 beginResult=-1;assert(!launcherDrawGpu(-1,255) && !scene);beginResult=0;
 uniformResult=-1;assert(!launcherDrawGpu(-1,255) && !scene);uniformResult=0;
 g_launcherPresentedGxm=false;displayResult=-1;
 assert(!launcherDrawGpu(-1,255) && !g_launcherPresentedGxm);displayResult=0;
 assert(launcherPrepareGpu(all));assert(launcherDrawGpu(2,255));
 const auto fadeStart=waits,fadeCompose=compositions;
 for(int frame=0;frame<=30;++frame){launcher_fade(frame,30);assert(g_launcherPresentedGxm);end_frame();}
 assert(waits-fadeStart==31 && compositions==fadeCompose && mapped.empty());
 assert(!g_launcherGpuReady && g_debugVisible && !g_probeDisplayingGxm);
 for(unsigned failure=0;failure<6;++failure){
  g_launcherGpuFailed=false;failAllocation=int(allocations+failure);
  assert(!launcherPrepareGpu(all) && mapped.empty() && g_debugVisible && !g_launcherGpuReady);
 }
 failAllocation=-1;g_launcherGpuFailed=false;
 assert(launcherPrepareGpu(all));assert(launcherDrawGpu(1,255));
 beginResult=-1;launcher_fade(0,30);beginResult=0;
 assert(mapped.empty() && g_debugVisible && !g_probeDisplayingGxm);
 launcherReleaseGpu();assert(mapped.empty());
 std::puts("PASS: production GXM state preparation/texel copies, zero selection-time composition/allocation, availability changes, 31 fade presents, resource cleanup and injected failures; no device FPS claim");
}
